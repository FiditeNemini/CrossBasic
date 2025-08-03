// MortgageFunctionsDemo.xs
// CrossBasic Demonstration Script for MortgageFunctions Plugin
// This script showcases the mortgage calculations and amortization schedule using:
//   - CalculateMonthlyPayment
//   - CalculateTotalPayment
//   - CalculateTotalInterest
//   - AmortizationSchedule

// ──────────────────────────────────────────────────────────────────────────────
// FUNCTION DEFINITIONS
// ──────────────────────────────────────────────────────────────────────────────

Function CalculateMonthlyPayment(principal As Double, annualRate As Double, loanTermYears As Integer) As Double
  Dim monthlyRate As Double = annualRate / 100.0 / 12.0
  Dim numberOfPayments As Integer = loanTermYears * 12
  
  If monthlyRate = 0 Then
    Return principal / numberOfPayments
  End If
  
  Dim denominator As Double = 1.0 - (1.0 + monthlyRate) ^ (-numberOfPayments)
  Return principal * monthlyRate / denominator
End Function

Function CalculateTotalPayment(monthlyPayment As Double, loanTermYears As Integer) As Double
  Return monthlyPayment * loanTermYears * 12
End Function

Function CalculateTotalInterest(principal As Double, totalPayment As Double) As Double
  Return totalPayment - principal
End Function

Function AmortizationSchedule(principal As Double, annualRate As Double, loanTermYears As Integer) As String
  Dim monthlyRate As Double = annualRate / 100.0 / 12.0
  Dim numberOfPayments As Integer = loanTermYears * 12
  Dim monthlyPayment As Double = CalculateMonthlyPayment(principal, annualRate, loanTermYears)
  Dim balance As Double = principal
  
  // Build header row (tab-separated)
  Dim sched As String = "Payment" + chr(9) + "PaymentAmount" + chr(9) + "Interest" + chr(9) + "Principal" + chr(9) + "Balance" + Chr(10)
  
  For i As Integer = 1 To numberOfPayments
    Dim interestPayment As Double = balance * monthlyRate
    Dim principalPayment As Double = monthlyPayment - interestPayment
    
    // Adjust final payment if rounding would overshoot
    If principalPayment > balance Then
      principalPayment = balance
    End If
    
    Dim actualPayment As Double = interestPayment + principalPayment
    balance = balance - principalPayment
    
    // Round to cents
    Dim pmtRounded As Double = Round(actualPayment * 100.0) / 100.0
    Dim intRounded As Double = Round(interestPayment * 100.0) / 100.0
    Dim prinRounded As Double = Round(principalPayment * 100.0) / 100.0
    Dim balRounded As Double = Round(balance * 100.0) / 100.0
    
    sched = sched _
      + Str(i) + Chr(9) _
      + Str(pmtRounded) + Chr(9) _
      + Str(intRounded) + Chr(9) _
      + Str(prinRounded) + Chr(9) _
      + Str(balRounded) + Chr(10)
  Next
  
  Return sched
End Function



// ──────────────────────────────────────────────────────────────────────────────
// MAIN DEMO SCRIPT
// ──────────────────────────────────────────────────────────────────────────────

// Ask user for principal
Print("Enter the principal amount (e.g. 200000.0):")
Dim principalInput As String = Input()
Dim principal As Double = Val(principalInput)

// Ask user for annual interest rate
Print("Enter the annual interest rate (percentage, e.g. 5.0):")
Dim rateInput As String = Input()
Dim annualRate As Double = Val(rateInput)

// Ask user for loan term in years
Print("Enter the loan term in years (e.g. 30):")
Dim termInput As String = Input()
Dim loanTermYears As Integer = Val(termInput)

// Store starting ticks for execution timing
Dim StartTime As Double = ticks
Print("Starting ticks: " + Str(StartTime))

// Perform calculations
Dim monthlyPayment As Double = CalculateMonthlyPayment(principal, annualRate, loanTermYears)
Print("Monthly Payment: $" + Str(Round(monthlyPayment * 100.0) / 100.0))

Dim totalPayment As Double = CalculateTotalPayment(monthlyPayment, loanTermYears)
Print("Total Payment: $" + Str(Round(totalPayment * 100.0) / 100.0))

Dim totalInterest As Double = CalculateTotalInterest(principal, totalPayment)
Print("Total Interest Paid: $" + Str(Round(totalInterest * 100.0) / 100.0))

// Generate and print amortization schedule
Print("Amortization Schedule:")
Dim schedule As String = AmortizationSchedule(principal, annualRate, loanTermYears)
Print(schedule)

// Save schedule to CSV
Dim tOut As New TextOutputStream
tOut.FilePath = "amortizationschedule.csv"
tOut.Append = False

If tOut.Open() = False Then
  Print("Failed to open file.")
  Return 0
End If

tOut.Write(ReplaceAll(schedule, Chr(9), ","))
tOut.Flush()
tOut.Close()
Print("Amortization schedule CSV saved successfully.")

// Timing runtime
Dim EndTime As Double = ticks
Dim elapsedSeconds As Double = (EndTime - StartTime) / 60.0  // adjust if ticks are not milliseconds
Print("Run Time: " + Str(Round(elapsedSeconds * 100.0) / 100.0) + " seconds")
