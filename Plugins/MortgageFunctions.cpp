/*

  MortgageFunctions.cpp
  CrossBasic Plugin: MortgageFunctions                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
    CrossBasic cross-platform (Windows/macOS/Linux) Function-based Plugin Example for Mortgage and Loan calculations
    
*/ 

#include <cstdio>
#include <string>
#include <cmath>    // For pow()

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

    // Plugin‐entry descriptor
    typedef struct PluginEntry {
        const char* name;
        void*       funcPtr;
        int         arity;
        const char* paramTypes[10];
        const char* retType;
    } PluginEntry;

    // 1) Monthly payment M = P * [ r(1+r)^n ] / [ (1+r)^n − 1 ]
    XPLUGIN_API double CalculateMonthlyPayment(double principal, double annualRate, int loanTermYears) {
        double monthlyRate   = annualRate / 12.0 / 100.0;
        int    totalPayments = loanTermYears * 12;
        return principal
             * monthlyRate
             * std::pow(1 + monthlyRate, totalPayments)
             / (std::pow(1 + monthlyRate, totalPayments) - 1);
    }

    // 2) Total payment over life of loan
    XPLUGIN_API double CalculateTotalPayment(double monthlyPayment, int loanTermYears) {
        return monthlyPayment * loanTermYears * 12;
    }

    // 3) Total interest = total paid − principal
    XPLUGIN_API double CalculateTotalInterest(double principal, double totalPayment) {
        return totalPayment - principal;
    }

    // 4) Amortization schedule as a big tab-separated string
    XPLUGIN_API const char* AmortizationSchedule(double principal, double annualRate, int loanTermYears) {
        static std::string schedule;
        schedule.clear();

        double monthlyRate   = annualRate / 12.0 / 100.0;
        int    totalPayments = loanTermYears * 12;
        double balance       = principal;

        // Header row
        schedule += "Month\tPayment\tPrincipal\tInterest\tBalance\n";

        for (int month = 1; month <= totalPayments; ++month) {
            double interest         = balance * monthlyRate;
            double payment          = CalculateMonthlyPayment(principal, annualRate, loanTermYears);
            double principalPortion = payment - interest;

            if (balance < payment) {
                principalPortion = balance;
                payment          = principalPortion + interest;
            }

            balance -= principalPortion;
            if (balance < 0.01) balance = 0.0;  // avoid tiny negatives

            char buffer[256];
            std::snprintf(buffer, sizeof(buffer),
                          "%d\t%.2f\t%.2f\t%.2f\t%.2f\n",
                          month, payment, principalPortion, interest, balance);
            schedule += buffer;
        }

        return schedule.c_str();
    }

    // Table of all plugin functions
    static PluginEntry pluginEntries[] = {
        { "CalculateMonthlyPayment", (void*)CalculateMonthlyPayment, 3, {"double","double","integer"}, "double" },
        { "CalculateTotalPayment",   (void*)CalculateTotalPayment,   2, {"double","integer"},       "double" },
        { "CalculateTotalInterest",  (void*)CalculateTotalInterest,  2, {"double","double"},        "double" },
        { "AmortizationSchedule",    (void*)AmortizationSchedule,    3, {"double","double","integer"}, "string" }
    };

    // Exported entry‐point for the host to discover our functions
    XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
        if (count) *count = static_cast<int>(sizeof(pluginEntries) / sizeof(PluginEntry));
        return pluginEntries;
    }

} // extern "C"

#ifdef _WIN32
  // Standard DLL entry point; no special init required here
  BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD /*reason*/, LPVOID /*reserved*/) {
      return TRUE;
  }
#endif
