// -----------------------------------------------------------------------------
// Demo: Conditional Logic and Comparison Tests in CrossBasic
// This script tests a variety of conditional expressions, including:
// - Integer, Double, and Boolean comparisons
// - Object equality checks
// - Color literal comparison
// -----------------------------------------------------------------------------

// Define color variables for literal comparison
Dim v As Color = &c00FF00
Dim b As Color = &c00FFF0

// Define booleans
Dim b1 As Boolean = True
Dim b2 As Boolean = False

// Define integers
Dim a As Integer = 20
Dim b As Integer = 5

// Define a float and an integer for comparison
Dim l As Double = 2.00
Dim t As Integer = 2

// Float and integer equality
If l = t Then 
  Print("l is 2.00 and t is 2. They are equal")
End If

// Check if a equals 20
If a = 20 Then 
  Print("A = 20 - test passed.")
End If

// Integer/Boolean combination and NOT logic
If a / 5 = 4 And Not b1 Then
  Print("Pass 1")
Else
  Print("Pass 2")
End If

// Boolean inequality check
If b1 <> b2 Then 
  Print("True is not equal to False")
End If

// Multi-condition using AND
If (b1 <> b2) And l = 2 And t = 2.00 Then 
  Print("These all pass the IF conditions.")
End If

// Multi-condition using OR
If (b1 = b2) Or (l = 2 And t = 2.00) Then 
  Print("These all pass the IF conditions.")
End If

// Color literal comparison (equality)
If v = v Then 
  Print(Str(v) + " = " + Str(&c00FFF0))  // Always true, but showing usage
Else
  Print("v = " + Str(v))
  Print("V is not equal to V")
End If

// Object reference comparison
Dim x As New XScreen
Dim y As New XScreen

// Compare an object to itself (reference equality)
If x = x Then
  Print("x = x")
Else
  Print("x <> x")
End If

// Compare two distinct instances
If x <> y Then 
  Print("X is not equal to Y")
End If
