/*

  sandbox.cpp
  CrossBasic Sandbox: sandbox                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
  Copyright (c) 2025 Simulanics Technologies – Matthew Combatti
  All rights reserved.
 
  Licensed under the CrossBasic Source License (CBSL-1.1).
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at:
  https://www.crossbasic.com/license
 
  SPDX-License-Identifier: CBSL-1.1
  
  Author:
    The AI Team under direction of Matthew Combatti <mcombatti@crossbasic.com>
    
*/ 
//
// Usage:
//   sandbox.exe <path_to_executable> [--dirs <path1,path2,...>]
//
// Examples:
//   // Only allow the EXE to access its own folder
//   sandbox.exe C:\Tools\test.exe
//
//   // Allow the EXE plus additional folders
//   sandbox.exe C:\Tools\test.exe --dirs C:\Data,C:\Logs
//
// Compile with MinGW-w64 g++ (ensure you have userenv, advapi32, ole32):
//   g++ -s -static -m64 -municode -std=c++17 -static-libgcc -static-libstdc++ -O3 -march=native -mtune=native sandbox.cpp -luserenv -ladvapi32 -lole32 -o sandbox.exe

#include <windows.h>
#include <userenv.h>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <objbase.h>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>

#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")

static std::wstring Trim(const std::wstring &s) {
    const wchar_t* ws = L" \t\r\n";
    size_t b = s.find_first_not_of(ws), e = s.find_last_not_of(ws);
    return (b == std::wstring::npos)
        ? L""
        : s.substr(b, e - b + 1);
}

static std::wstring MakeGuid() {
    GUID g;
    CoCreateGuid(&g);
    wchar_t *txt = nullptr;
    StringFromCLSID(g, &txt);
    std::wstring s(txt);
    CoTaskMemFree(txt);
    if (s.size()>1 && s.front()==L'{' && s.back()==L'}')
        return s.substr(1, s.size()-2);
    return s;
}

// Grant GENERIC_ALL to pSid on folder path
static void AllowAppContainerOnFolder(PSID pSid, const std::wstring &folder) {
    PSECURITY_DESCRIPTOR pSD = nullptr;
    PACL pOldDACL = nullptr;
    if (GetNamedSecurityInfoW(
            folder.c_str(),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr, nullptr,
            &pOldDACL,
            nullptr,
            &pSD) != ERROR_SUCCESS)
    {
        throw std::runtime_error("GetNamedSecurityInfoW failed");
    }

    EXPLICIT_ACCESSW ea{};
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode        = SET_ACCESS;
    ea.grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType  = TRUSTEE_IS_USER;
    ea.Trustee.ptstrName    = (LPWSTR)pSid;

    PACL pNewDACL = nullptr;
    if (SetEntriesInAclW(1, &ea, pOldDACL, &pNewDACL) != ERROR_SUCCESS) {
        LocalFree(pSD);
        throw std::runtime_error("SetEntriesInAclW failed");
    }

    if (SetNamedSecurityInfoW(
            const_cast<LPWSTR>(folder.c_str()),
            SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION,
            nullptr, nullptr,
            pNewDACL,
            nullptr) != ERROR_SUCCESS)
    {
        LocalFree(pSD);
        LocalFree(pNewDACL);
        throw std::runtime_error("SetNamedSecurityInfoW failed");
    }

    LocalFree(pSD);
    LocalFree(pNewDACL);
}

int wmain(int argc, wchar_t *argv[]) {
    if (argc != 2 && argc != 4) {
        std::wcerr << L"Usage: sandbox.exe <exe> [--dirs <p1,p2,...>]\n";
        return 1;
    }

    std::wstring exePath = argv[1];
    std::vector<std::wstring> folders;

    if (argc == 4) {
        if (std::wstring(argv[2]) != L"--dirs") {
            std::wcerr << L"Unknown flag: " << argv[2] << L"\n";
            return 1;
        }
        std::wstring all = argv[3];
        size_t pos = 0;
        while (pos < all.size()) {
            size_t c = all.find(L',', pos);
            if (c == std::wstring::npos) c = all.size();
            auto f = Trim(all.substr(pos, c - pos));
            if (!f.empty()) folders.push_back(f);
            pos = c + 1;
        }
    }

    // Always allow the EXE's own directory
    {
        auto dir = std::filesystem::path(exePath)
                       .parent_path()
                       .wstring();
        if (!dir.empty() &&
            std::find(folders.begin(), folders.end(), dir) == folders.end())
        {
            folders.push_back(dir);
        }
    }

    // Initialize COM
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Create AppContainer profile
    std::wstring profileName = L"Sbx_" + MakeGuid();
    PSID pAppSid = nullptr;
    HRESULT hr = CreateAppContainerProfile(
        profileName.c_str(),
        profileName.c_str(),
        L"Auto-generated sandbox profile",
        nullptr, 0,
        &pAppSid);

    if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
        DeriveAppContainerSidFromAppContainerName(
            profileName.c_str(),
            &pAppSid);
    }
    else if (FAILED(hr)) {
        std::wcerr << L"CreateAppContainerProfile failed: 0x"
                   << std::hex << hr << L"\n";
        return 1;
    }

    // ACL each folder
    try {
        for (auto &f : folders) {
            AllowAppContainerOnFolder(pAppSid, f);
        }
    }
    catch (const std::exception &e) {
        std::wcerr << L"ACL error: " << e.what() << L"\n";
        DeleteAppContainerProfile(profileName.c_str());
        return 1;
    }

    // Prepare ProcThreadAttributeList
    SIZE_T attrSize = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &attrSize);
    auto attr = (LPPROC_THREAD_ATTRIBUTE_LIST)
        HeapAlloc(GetProcessHeap(), 0, attrSize);
    InitializeProcThreadAttributeList(attr, 1, 0, &attrSize);

    SECURITY_CAPABILITIES caps{};
    caps.AppContainerSid = pAppSid;
    UpdateProcThreadAttribute(
        attr,
        0,
        PROC_THREAD_ATTRIBUTE_SECURITY_CAPABILITIES,
        &caps, sizeof(caps),
        nullptr, nullptr);

    // Build command line
    std::filesystem::path p(exePath);
    std::wstring ext = p.extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), towlower);

    std::wstring cmdLine;
    if (ext == L".bat" || ext == L".cmd") {
        cmdLine = L"cmd.exe /C \"" + exePath + L"\"";
    }
    else {
        cmdLine = L"\"" + exePath + L"\"";
    }

    // Copy to modifiable buffer
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    // Launch the sandboxed process
    STARTUPINFOEXW siex{ sizeof(siex) };
    siex.lpAttributeList = attr;
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(
            nullptr,
            cmdBuf.data(),
            nullptr, nullptr,
            FALSE,
            EXTENDED_STARTUPINFO_PRESENT,
            nullptr, nullptr,
            &siex.StartupInfo,
            &pi))
    {
        std::wcerr << L"CreateProcessW failed: "
                   << GetLastError() << L"\n";
        DeleteProcThreadAttributeList(attr);
        HeapFree(GetProcessHeap(), 0, attr);
        DeleteAppContainerProfile(profileName.c_str());
        return 1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // Cleanup
    DeleteProcThreadAttributeList(attr);
    HeapFree(GetProcessHeap(), 0, attr);
    DeleteAppContainerProfile(profileName.c_str());
    CoUninitialize();

    return 0;
}
