/*

  SimCipher.cpp
  CrossBasic Plugin: SimCipher                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
// Build Commands:
//   Windows: g++ -static -static-libgcc -static-libstdc++ -shared -o SimCipher.dll SimCipherClass.cpp
//   Linux/macOS: g++ -static -shared -fPIC -o SimCipherPlugin.so SimCipherClass.cpp
//
// Dependencies: Boost.Multiprecision
//
// This plugin implements RSA-like key generation, encryption, decryption, signing, and verifying
// using S-box transformations and a built-in minimal SHA-256 implementation.

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <stdexcept>
#include <numeric>
#include <algorithm>
#include <functional>
#include <mutex>
#include <map>
#include <atomic>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #include <unistd.h>
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

#include <boost/multiprecision/cpp_int.hpp>
using namespace std;
using namespace boost::multiprecision;

// -----------------------------------------------------------------------------
// Minimal SHA-256 Implementation (Single Definition)
// -----------------------------------------------------------------------------

// SHA-256 constants (64 values)
const uint32_t k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
    0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
    0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
    0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
    0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
    0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ (~e & g);
}

uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

uint32_t sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

uint32_t sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

uint32_t theta0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

uint32_t theta1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

class SHA256 {
public:
    SHA256() { reset(); }

    void update(const unsigned char* data, size_t len) {
        while (len > 0) {
            size_t to_copy = min(len, size_t(64) - buffer.size());
            buffer.insert(buffer.end(), data, data + to_copy);
            data += to_copy;
            len -= to_copy;
            if (buffer.size() == 64) {
                transform();
                total_length += 512;
                buffer.clear();
            }
        }
    }
    void update(const vector<unsigned char>& data) { update(data.data(), data.size()); }
    void update(const string& data) { update(reinterpret_cast<const unsigned char*>(data.c_str()), data.size()); }

    vector<unsigned char> digest() {
        vector<unsigned char> padded = buffer;
        padded.push_back(0x80);
        while (padded.size() % 64 != 56)
            padded.push_back(0x00);
        uint64_t total_bits = total_length + buffer.size() * 8;
        for (int i = 7; i >= 0; --i)
            padded.push_back(static_cast<unsigned char>((total_bits >> (i * 8)) & 0xFF));
        for (size_t i = 0; i < padded.size(); i += 64) {
            vector<unsigned char> block(padded.begin() + i, padded.begin() + i + 64);
            buffer = block;
            transform();
            total_length += 512;
        }
        vector<unsigned char> hash;
        for (int i = 0; i < 8; ++i) {
            uint32_t word = h[i];
            hash.push_back((word >> 24) & 0xFF);
            hash.push_back((word >> 16) & 0xFF);
            hash.push_back((word >> 8) & 0xFF);
            hash.push_back(word & 0xFF);
        }
        reset();
        return hash;
    }

    string toString(const vector<unsigned char>& hash) const {
        stringstream ss;
        ss << hex << setfill('0');
        for (auto byte : hash) {
            ss << setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

private:
    void reset() {
        h[0] = 0x6a09e667;
        h[1] = 0xbb67ae85;
        h[2] = 0x3c6ef372;
        h[3] = 0xa54ff53a;
        h[4] = 0x510e527f;
        h[5] = 0x9b05688c;
        h[6] = 0x1f83d9ab;
        h[7] = 0x5be0cd19;
        buffer.clear();
        total_length = 0;
    }

    void transform() {
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        uint32_t W[64];
        for (int i = 0; i < 16; ++i) {
            W[i] = (buffer[i*4] << 24) | (buffer[i*4+1] << 16) | (buffer[i*4+2] << 8) | (buffer[i*4+3]);
        }
        for (int i = 16; i < 64; ++i) {
            W[i] = theta1(W[i-2]) + W[i-7] + theta0(W[i-15]) + W[i-16];
        }
        for (int i = 0; i < 64; ++i) {
            uint32_t T1 = hh + sig1(e) + choose(e, f, g) + k[i] + W[i];
            uint32_t T2 = sig0(a) + majority(a, b, c);
            hh = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    uint32_t h[8];
    vector<unsigned char> buffer;
    uint64_t total_length;
};

// -----------------------------------------------------------------------------
// Helper Functions for Byte Conversions
// -----------------------------------------------------------------------------

string bytes_to_hex(const vector<unsigned char>& bytes) {
    stringstream ss;
    ss << hex << setfill('0');
    for (auto byte : bytes)
        ss << setw(2) << static_cast<int>(byte);
    return ss.str();
}

vector<unsigned char> hex_to_bytes(const string& hex) {
    vector<unsigned char> bytes;
    if (hex.size() % 2 != 0)
        return {};
    for (size_t i = 0; i < hex.size(); i += 2)
        bytes.push_back(static_cast<unsigned char>(stoul(hex.substr(i, 2), nullptr, 16)));
    return bytes;
}

cpp_int bytes_to_int(const vector<unsigned char>& bytes) {
    cpp_int num = 0;
    for (auto byte : bytes)
        num = (num << 8) | byte;
    return num;
}

vector<unsigned char> int_to_bytes(cpp_int num) {
    vector<unsigned char> bytes;
    while (num > 0) {
        bytes.insert(bytes.begin(), static_cast<unsigned char>(num & 0xFF));
        num >>= 8;
    }
    if (bytes.empty())
        bytes.push_back(0);
    return bytes;
}

cpp_int hex_str_to_cpp_int(const string& hex) {
    cpp_int result;
    stringstream ss;
    ss << hex;
    ss >> std::hex >> result;
    return result;
}

// =====================================================================
// SimCipher3 Class (Plugin Class)
// =====================================================================

class SimCipher3 {
public:
    // Parameterless constructor; default rounds is 4.
    SimCipher3(int rounds_ = 4) : rounds(rounds_) {
        sbox_a = initialize_sbox();
        sbox_b = initialize_sbox();
        sbox_a_inv.resize(256, 0);
        sbox_b_inv.resize(256, 0);
        for (int i = 0; i < 256; ++i) {
            sbox_a_inv[sbox_a[i]] = i;
            sbox_b_inv[sbox_b[i]] = i;
        }
        // Keys not set initially.
        p = q = n = e = d = 0;
    }

    // Now generate_keys takes an integer parameter for key length.
    void generate_keys(int bits = 1024) {
        p = generate_prime(bits / 2);
        q = generate_prime(bits / 2);
        n = p * q;
        e = 65537;
        d = modinv(e, (p - 1) * (q - 1));
    }

    string get_public_key() {
        return n.str(16) + "|" + e.str(16);
    }

    string get_private_key() {
        return n.str(16) + "|" + d.str(16);
    }

    void load_keys(const string& public_key, const string& private_key) {
        size_t pos = public_key.find("|");
        if (pos == string::npos)
            throw invalid_argument("Invalid public key format");
        string n_hex = public_key.substr(0, pos);
        string e_hex = public_key.substr(pos + 1);

        pos = private_key.find("|");
        if (pos == string::npos)
            throw invalid_argument("Invalid private key format");
        string n_hex2 = private_key.substr(0, pos);
        string d_hex = private_key.substr(pos + 1);

        if (n_hex != n_hex2)
            throw invalid_argument("Public and private keys do not match");

        n = hex_str_to_cpp_int(n_hex);
        e = hex_str_to_cpp_int(e_hex);
        d = hex_str_to_cpp_int(d_hex);
    }

    // Encrypts the plaintext (as a string) and returns a hex ciphertext.
    string encrypt(const string& plaintext) {
        vector<unsigned char> pt(plaintext.begin(), plaintext.end());
        cpp_int m = bytes_to_int(pt);
        if (m >= n)
            return "ERROR";
        cpp_int c = powm(m, e, n);
        vector<unsigned char> c_bytes = int_to_bytes(c);
        vector<unsigned char> transformed = apply_sbox_encryption(c_bytes);
        return bytes_to_hex(transformed);
    }

    // Decrypts the hex ciphertext and returns the plaintext.
    string decrypt(const string& ciphertext_hex) {
        vector<unsigned char> c_bytes = hex_to_bytes(ciphertext_hex);
        vector<unsigned char> reversed = apply_sbox_decryption(c_bytes);
        cpp_int c = bytes_to_int(reversed);
        cpp_int m = powm(c, d, n);
        vector<unsigned char> pt = int_to_bytes(m);
        return string(pt.begin(), pt.end());
    }

    // Signs the message (as a string) and returns the signature as a hex string.
    string sign(const string& message) {
        vector<unsigned char> msg(message.begin(), message.end());
        vector<unsigned char> hash = sha256(msg);
        cpp_int h_int = bytes_to_int(hash);
        cpp_int s = powm(h_int, d, n);
        vector<unsigned char> sig_bytes = int_to_bytes(s);
        return bytes_to_hex(sig_bytes);
    }

    // Verifies the signature (hex string) for the given message; returns "true" or "false".
    string verify(const string& message, const string& signature_hex) {
        vector<unsigned char> msg(message.begin(), message.end());
        vector<unsigned char> hash = sha256(msg);
        cpp_int h_int = bytes_to_int(hash);
        vector<unsigned char> sig_bytes = hex_to_bytes(signature_hex);
        cpp_int s_int = bytes_to_int(sig_bytes);
        cpp_int recovered = powm(s_int, e, n);
        return (recovered == h_int) ? "true" : "false";
    }

    string ToString() {
        return "SimCipher3:" + to_string(rounds);
    }

private:
    int rounds;
    vector<uint8_t> sbox_a, sbox_b, sbox_a_inv, sbox_b_inv;
    cpp_int p, q, n, e, d;

    static cpp_int generate_prime(unsigned int bits) {
        while (true) {
            cpp_int candidate = generate_random(bits);
            if (candidate % 2 == 0)
                candidate += 1;
            if (is_probably_prime(candidate, 20))
                return candidate;
        }
    }

    static cpp_int generate_random(unsigned int bits) {
        int byteCount = (bits + 7) / 8;
        vector<unsigned char> bytes(byteCount);
        random_device rd;
        for (int i = 0; i < byteCount; ++i)
            bytes[i] = static_cast<unsigned char>(rd() & 0xFF);
        bytes[0] |= (1 << ((bits - 1) % 8));
        return bytes_to_int(bytes);
    }

    static bool is_probably_prime(const cpp_int &n, int iterations) {
        if (n < 2)
            return false;
        if (n == 2 || n == 3)
            return true;
        if (n % 2 == 0)
            return false;
        cpp_int d = n - 1;
        int s = 0;
        while (d % 2 == 0) {
            d /= 2;
            s++;
        }
        for (int i = 0; i < iterations; i++) {
            cpp_int a = 2 + (generate_random(64) % (n - 3));
            cpp_int x = powm(a, d, n);
            if (x == 1 || x == n - 1)
                continue;
            bool cont = false;
            for (int r = 1; r < s; r++) {
                x = (x * x) % n;
                if (x == n - 1) {
                    cont = true;
                    break;
                }
            }
            if (!cont)
                return false;
        }
        return true;
    }

    static cpp_int powm(const cpp_int& base, const cpp_int& exp, const cpp_int& mod) {
        cpp_int result = 1, b = base % mod, e = exp;
        while (e > 0) {
            if (e % 2 == 1)
                result = (result * b) % mod;
            b = (b * b) % mod;
            e /= 2;
        }
        return result;
    }

    static cpp_int egcd(const cpp_int& a, const cpp_int& b, cpp_int& x, cpp_int& y) {
        if (a == 0) {
            x = 0; y = 1;
            return b;
        }
        cpp_int x1, y1;
        cpp_int g = egcd(b % a, a, x1, y1);
        x = y1 - (b / a) * x1;
        y = x1;
        return g;
    }

    static cpp_int modinv(const cpp_int& a, const cpp_int& m) {
        cpp_int x, y;
        if (egcd(a, m, x, y) != 1)
            throw invalid_argument("Modular inverse does not exist");
        return (x % m + m) % m;
    }

    // SHA-256: Use our minimal implementation.
    static vector<unsigned char> sha256(const vector<unsigned char>& data) {
        SHA256 sha;
        sha.update(data);
        return sha.digest();
    }

    static string bytes_to_hex(const vector<unsigned char>& bytes) {
        stringstream ss;
        ss << hex << setw(2) << setfill('0');
        for (auto byte : bytes)
            ss << setw(2) << static_cast<int>(byte);
        return ss.str();
    }

    static vector<unsigned char> hex_to_bytes(const string& hex) {
        vector<unsigned char> bytes;
        if (hex.size() % 2 != 0)
            return {};
        for (size_t i = 0; i < hex.size(); i += 2)
            bytes.push_back(static_cast<unsigned char>(stoul(hex.substr(i, 2), nullptr, 16)));
        return bytes;
    }

    static cpp_int bytes_to_int(const vector<unsigned char>& bytes) {
        cpp_int num = 0;
        for (auto byte : bytes)
            num = (num << 8) | byte;
        return num;
    }

    static vector<unsigned char> int_to_bytes(cpp_int num) {
        vector<unsigned char> bytes;
        while (num > 0) {
            bytes.insert(bytes.begin(), static_cast<unsigned char>(num & 0xFF));
            num >>= 8;
        }
        if (bytes.empty())
            bytes.push_back(0);
        return bytes;
    }

    static cpp_int hex_str_to_cpp_int(const string& hex) {
        cpp_int result;
        stringstream ss;
        ss << hex;
        ss >> std::hex >> result;
        return result;
    }

    vector<uint8_t> initialize_sbox() const {
        vector<uint8_t> sbox(256);
        iota(sbox.begin(), sbox.end(), 0);
        random_device rd;
        mt19937 g(rd());
        shuffle(sbox.begin(), sbox.end(), g);
        return sbox;
    }

    vector<unsigned char> apply_sbox_encryption(vector<unsigned char> data) const {
        for (int r = 0; r < rounds; ++r) {
            for (size_t i = 0; i < data.size() - 1; ++i) {
                uint8_t a = data[i];
                uint8_t b = data[i+1];
                data[i] = sbox_b[a ^ b];
                data[i+1] = sbox_a[b ^ data[i]];
            }
        }
        return data;
    }

    vector<unsigned char> apply_sbox_decryption(vector<unsigned char> data) const {
        for (int r = 0; r < rounds; ++r) {
            for (size_t i = data.size() - 1; i > 0; --i) {
                uint8_t t_prev = data[i-1];
                uint8_t t_curr = data[i];
                uint8_t b = sbox_a_inv[t_curr] ^ t_prev;
                uint8_t a = sbox_b_inv[t_prev] ^ b;
                data[i-1] = a;
                data[i] = b;
            }
        }
        return data;
    }
};

// =====================================================================
// Global Instance Management for SimCipher3 Instances
// =====================================================================
static mutex cipherMutex;
static map<int, SimCipher3*> cipherMap;
static atomic<int> nextCipherHandle(1);

// =====================================================================
// Exported Functions for SimCipher (Class-Based Plugin)
// =====================================================================

// Constructor: Creates a new SimCipher3 instance and returns its unique handle.
extern "C" XPLUGIN_API int NewSimCipher() {
    SimCipher3* cipher = new SimCipher3();
    int handle = nextCipherHandle.fetch_add(1);
    {
        lock_guard<mutex> lock(cipherMutex);
        cipherMap[handle] = cipher;
    }
    return handle;
}

// Generates new keys using the provided bit length and returns them as "public|private".
// public key = "n|e", private key = "n|d"
extern "C" XPLUGIN_API const char* SimCipher_GenerateKeys(int handle, int bits) {
    static string keys;
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end())
        return "Error: Invalid instance handle";
    it->second->generate_keys(bits);
    keys = it->second->get_public_key() + "|" + it->second->get_private_key();
    return keys.c_str();
}

// Loads keys into the instance.
extern "C" XPLUGIN_API const char* SimCipher_LoadKeys(int handle, const char* public_key, const char* private_key) {
    static string result;
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end())
        return "Error: Invalid instance handle";
    try {
        it->second->load_keys(string(public_key), string(private_key));
        result = "Keys loaded successfully";
    } catch (const exception &e) {
        result = e.what();
    }
    return result.c_str();
}

// Encrypts a plaintext message; returns a hex string.
extern "C" XPLUGIN_API const char* SimCipher_EncryptMessage(int handle, const char* plaintext) {
    static string ciphertext;
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end())
        return "Error: Invalid instance handle";
    ciphertext = it->second->encrypt(string(plaintext));
    return ciphertext.c_str();
}

// Decrypts a ciphertext (hex string) and returns the plaintext.
extern "C" XPLUGIN_API const char* SimCipher_DecryptMessage(int handle, const char* ciphertext_hex) {
    static string plaintext;
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end())
        return "Error: Invalid instance handle";
    plaintext = it->second->decrypt(string(ciphertext_hex));
    return plaintext.c_str();
}

// Signs a message; returns the signature as a hex string.
extern "C" XPLUGIN_API const char* SimCipher_SignMessage(int handle, const char* message) {
    static string signature;
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end())
        return "Error: Invalid instance handle";
    signature = it->second->sign(string(message));
    return signature.c_str();
}

// Verifies a signature; returns "true" or "false".
extern "C" XPLUGIN_API const char* SimCipher_VerifySignature(int handle, const char* message, const char* signature_hex) {
    static string result;
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end())
        return "Error: Invalid instance handle";
    result = it->second->verify(string(message), string(signature_hex));
    return result.c_str();
}

// Destroys a SimCipher instance.
extern "C" XPLUGIN_API bool SimCipher_Destroy(int handle) {
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    if (it == cipherMap.end()) return false;
    delete it->second;
    cipherMap.erase(it);
    return true;
}

// =====================================================================
// Class Definition Structures for SimCipher (for interpreter integration)
// =====================================================================

typedef struct {
    const char* name;
    const char* type;
    void* getter;
    void* setter;
} ClassProperty;

typedef struct {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} ClassEntry;

typedef struct {
    const char* declaration;
} ClassConstant;

typedef struct {
    const char* className;
    size_t classSize;
    void* constructor;
    ClassProperty* properties;
    size_t propertiesCount;
    ClassEntry* methods;
    size_t methodsCount;
    ClassConstant* constants;
    size_t constantsCount;
} ClassDefinition;

// Standalone getter for the ToString property.
extern "C" const char* SimCipher_ToStringGetter(int handle) {
    lock_guard<mutex> lock(cipherMutex);
    auto it = cipherMap.find(handle);
    static string s;
    if (it != cipherMap.end()) {
        s = it->second->ToString();
        return s.c_str();
    }
    return "";
}

static ClassProperty SimCipherProperties[] = {
    { "ToString", "string", (void*)SimCipher_ToStringGetter, nullptr }
};

static ClassEntry SimCipherMethods[] = {
    // Now GenerateKeys takes 2 parameters: instance handle and integer key length.
    { "GenerateKeys", (void*)SimCipher_GenerateKeys, 2, {"integer", "integer"}, "string" },
    { "LoadKeys", (void*)SimCipher_LoadKeys, 3, {"integer", "string", "string"}, "string" },
    { "EncryptMessage", (void*)SimCipher_EncryptMessage, 2, {"integer", "string"}, "string" },
    { "DecryptMessage", (void*)SimCipher_DecryptMessage, 2, {"integer", "string"}, "string" },
    { "SignMessage", (void*)SimCipher_SignMessage, 2, {"integer", "string"}, "string" },
    { "VerifySignature", (void*)SimCipher_VerifySignature, 3, {"integer", "string", "string"}, "string" },
    { "Close", (void*)SimCipher_Destroy, 1, {"integer"}, "boolean" }
};

static ClassConstant SimCipherConstants[] = { };

static ClassDefinition SimCipherClass = {
    "SimCipher",
    sizeof(SimCipher3),
    (void*)NewSimCipher, // Parameterless constructor.
    SimCipherProperties,
    sizeof(SimCipherProperties) / sizeof(ClassProperty),
    SimCipherMethods,
    sizeof(SimCipherMethods) / sizeof(ClassEntry),
    SimCipherConstants,
    sizeof(SimCipherConstants) / sizeof(ClassConstant)
};

extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &SimCipherClass;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
#endif
