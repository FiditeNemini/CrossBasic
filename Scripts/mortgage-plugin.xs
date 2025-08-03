// -------------------------------------------------------------------------------
// CrossBasic Demonstration Script for MortgageFunctions Plugin
// This script showcases the mortgage calculations and amortization schedule using:
// - CalculateMonthlyPayment
// - CalculateTotalPayment
// - CalculateTotalInterest
// - AmortizationSchedule
// -------------------------------------------------------------------------------

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
Dim StartTime As Double
StartTime = ticks
// Print tick time
Print("Starting ticks: " + Str(StartTime))

// Calculate and print monthly payment
Dim monthlyPayment As Double = CalculateMonthlyPayment(principal, annualRate, loanTermYears)
Print("Monthly Payment: $" + Str(Round(monthlyPayment)))

// Calculate and print total payment over the life of the loan
Dim totalPayment As Double = CalculateTotalPayment(monthlyPayment, loanTermYears)
Print("Total Payment: $" + Str(Round(totalPayment)))

// Calculate and print total interest paid over the life of the loan
Dim totalInterest As Double = CalculateTotalInterest(principal, totalPayment)
Print("Total Interest Paid: $" + Str(Round(totalInterest)))

// Generate and print amortization schedule
Print("Amortization Schedule:")
Dim schedule As String = AmortizationSchedule(principal, annualRate, loanTermYears)
Print(schedule)

Dim tOut As New TextOutputStream
tOut.FilePath = "amortizationschedule.csv"
tOut.Append = False

If tOut.Open() = False Then
  Print("Failed to open file.")
  Return 0
End If

tOut.Write(ReplaceAll(schedule, chr(9), ","))
tOut.Flush()
tOut.Close()

Print("Amortization schedule CSV saved successfully.")


// Timing runtime
Dim EndTime As Double = ticks
Dim elapsedSeconds As Double = (EndTime - StartTime) / 60  // adjust divisor if ticks are in ms
Print("Run Time: " + Str(Round(elapsedSeconds)) + " seconds")
