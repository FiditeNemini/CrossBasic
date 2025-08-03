/*

  Speak.rs
  CrossBasic Plugin: Speak                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_void};
use std::ptr;
use std::process::Command;

#[no_mangle]
pub extern "C" fn speak(text: *const c_char) -> bool {
    if text.is_null() {
        return false;
    }
    let c_str = unsafe { CStr::from_ptr(text) };
    let text_str = match c_str.to_str() {
        Ok(s) => s,
        Err(_) => return false,
    };

    #[cfg(target_os = "windows")]
    {
        // Escape single quotes by doubling them.
        let escaped_text = text_str.replace("'", "''");
        // Build the PowerShell command.
        let command_str = format!(
            "Add-Type -AssemblyName System.Speech; (New-Object System.Speech.Synthesis.SpeechSynthesizer).Speak('{}')",
            escaped_text
        );
        let status = Command::new("powershell")
            .args(&["-Command", &command_str])
            .status();
        return status.map_or(false, |s| s.success());
    }

    #[cfg(target_os = "macos")]
    {
        // Use the macOS "say" command.
        let status = Command::new("say").arg(text_str).status();
        return status.map_or(false, |s| s.success());
    }

    #[cfg(target_os = "linux")]
    {
        // Attempt to use "spd-say"; if that fails, fallback to "espeak".
        let status = Command::new("spd-say").arg(text_str).status();
        if let Ok(s) = status {
            if s.success() {
                return true;
            }
        }
        let status = Command::new("espeak").arg(text_str).status();
        return status.map_or(false, |s| s.success());
    }

    #[cfg(not(any(target_os = "windows", target_os = "macos", target_os = "linux")))]
    {
        false
    }
}

#[repr(C)]
pub struct PluginEntry {
    name: *const c_char,            // Plugin name.
    func_ptr: *const c_void,        // Pointer to the exported function.
    arity: c_int,                   // Number of parameters.
    param_types: [*const c_char; 10], // Parameter datatypes.
    ret_type: *const c_char,        // Return datatype.
}

#[no_mangle]
pub static mut PLUGIN_ENTRIES: [PluginEntry; 1] = [
    PluginEntry {
        name: b"Speak\0".as_ptr() as *const c_char,
        func_ptr: speak as *const c_void,
        arity: 1,
        param_types: [
            b"string\0".as_ptr() as *const c_char,
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(), ptr::null(), ptr::null(), ptr::null(),
            ptr::null(),
        ],
        ret_type: b"boolean\0".as_ptr() as *const c_char,
    },
];

#[no_mangle]
pub extern "C" fn GetPluginEntries(count: *mut c_int) -> *mut PluginEntry {
    unsafe {
        if !count.is_null() {
            *count = PLUGIN_ENTRIES.len() as c_int;
        }
        PLUGIN_ENTRIES.as_mut_ptr()
    }
}
