/*

  BigInteger.cpp
  CrossBasic Plugin: BigInteger
 
  Copyright (c) 2025 Simulanics Technologies – Matthew Combatti
  All rights reserved.
 
  Licensed under the CrossBasic Source License (CBSL-1.1).
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at:
  https://www.crossbasic.com/license
 
  SPDX-License-Identifier: CBSL-1.1
  
  Author:
    The AI Team under direction of Matthew Combatti <mcombatti@crossbasic.com>
	
  Description:
    Implementation of arbitrary-precision arithmetic bindings
    (BigIntAdd, BigIntSubtract, etc.) using GMP for CrossBasic.
 
  Build (Windows):
    g++ -shared -o BigInteger.dll BigInteger.cpp -lgmp -static-libgcc -static-libstdc++ -std=c++17 -O3 -s

*/

#include <gmp.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

// Duplicate a C-string (so we don't rely on non-standard strdup)
static char* duplicate_string(const char* s) {
    size_t len = strlen(s);
    char* d   = (char*)malloc(len + 1);
    if (!d) return nullptr;
    memcpy(d, s, len + 1);
    return d;
}

// Convert an mpf_t to a newly‐malloc’d C-string with 'precision' digits after the decimal point
char* convert_gmp_to_string(mpf_t value, int precision) {
    char* output = nullptr;
    // gmp_asprintf will malloc and fill 'output'; format %.*Ff prints fixed-point
    gmp_asprintf(&output, "%.*Ff", precision, value);
    return output;
}

// Addition
XPLUGIN_API const char* BigIntAdd(const char* n1, const char* n2, int precision) {
    mpf_t a, b, r;
    mpf_init2(a, precision * 4);
    mpf_init2(b, precision * 4);
    mpf_init2(r, precision * 4);

    mpf_set_str(a, n1, 10);
    mpf_set_str(b, n2, 10);
    mpf_add(r, a, b);

    char* out = convert_gmp_to_string(r, precision);
    mpf_clear(a); mpf_clear(b); mpf_clear(r);
    return out;
}

// Subtraction
XPLUGIN_API const char* BigIntSubtract(const char* n1, const char* n2, int precision) {
    mpf_t a, b, r;
    mpf_init2(a, precision * 4);
    mpf_init2(b, precision * 4);
    mpf_init2(r, precision * 4);

    mpf_set_str(a, n1, 10);
    mpf_set_str(b, n2, 10);
    mpf_sub(r, a, b);

    char* out = convert_gmp_to_string(r, precision);
    mpf_clear(a); mpf_clear(b); mpf_clear(r);
    return out;
}

// Multiplication
XPLUGIN_API const char* BigIntMultiply(const char* n1, const char* n2, int precision) {
    mpf_t a, b, r;
    mpf_init2(a, precision * 4);
    mpf_init2(b, precision * 4);
    mpf_init2(r, precision * 4);

    mpf_set_str(a, n1, 10);
    mpf_set_str(b, n2, 10);
    mpf_mul(r, a, b);

    char* out = convert_gmp_to_string(r, precision);
    mpf_clear(a); mpf_clear(b); mpf_clear(r);
    return out;
}

// Division
XPLUGIN_API const char* BigIntDivide(const char* n1, const char* n2, int precision) {
    mpf_t a, b, r;
    mpf_init2(a, precision * 4);
    mpf_init2(b, precision * 4);
    mpf_init2(r, precision * 4);

    mpf_set_str(a, n1, 10);
    mpf_set_str(b, n2, 10);
    if (mpf_cmp_ui(b, 0) == 0) {
        mpf_clear(a); mpf_clear(b); mpf_clear(r);
        return duplicate_string("ERROR: Division by zero");
    }
    mpf_div(r, a, b);

    char* out = convert_gmp_to_string(r, precision);
    mpf_clear(a); mpf_clear(b); mpf_clear(r);
    return out;
}

// Modulo (integer)
XPLUGIN_API const char* BigIntModulo(const char* n1, const char* n2, int /*precision*/) {
    mpz_t a, b, r;
    mpz_init_set_str(a, n1, 10);
    mpz_init_set_str(b, n2, 10);
    mpz_init(r);

    if (mpz_cmp_ui(b, 0) == 0) {
        mpz_clear(a); mpz_clear(b); mpz_clear(r);
        return duplicate_string("ERROR: Modulo by zero");
    }
    mpz_mod(r, a, b);

    char* out = mpz_get_str(nullptr, 10, r);
    mpz_clear(a); mpz_clear(b); mpz_clear(r);
    return out;
}

// Plugin registration
typedef struct {
    const char* name;
    void*       funcPtr;
    int         arity;
    const char* paramTypes[10];
    const char* retType;
} PluginEntry;

static PluginEntry pluginEntries[] = {
    { "BigIntAdd",      (void*)BigIntAdd,      3, {"string","string","integer"}, "string" },
    { "BigIntSubtract", (void*)BigIntSubtract, 3, {"string","string","integer"}, "string" },
    { "BigIntMultiply", (void*)BigIntMultiply, 3, {"string","string","integer"}, "string" },
    { "BigIntDivide",   (void*)BigIntDivide,   3, {"string","string","integer"}, "string" },
    { "BigIntModulo",   (void*)BigIntModulo,   3, {"string","string","integer"}, "string" }
};

XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) *count = sizeof(pluginEntries)/sizeof(PluginEntry);
    return pluginEntries;
}

} // extern "C"
