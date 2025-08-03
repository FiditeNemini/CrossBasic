// -----------------------------------------------------------------------------
// Demo: Using Modules, Classes, and Built-in Functions in CrossBasic
// This script demonstrates basic language features including:
// - Module definitions and usage
// - Class declaration with properties and methods
// - Control structures (If, For)
// - Color literals, math functions, and randomness
// -----------------------------------------------------------------------------

// Module: MathUtilities
Module MathUtilities
  // Function to multiply two integers
  Public Function Multiply(a As Integer, b As Integer) As Integer
    Return a * b
  End Function
End Module

// Class: Person
Class Person
  Var Name As String  // Property to store person's name

  // Constructor: initializes the Name property
  Sub Constructor(newName As String)
    Name = newName
  End Sub

  // Method: prints a greeting to the console
  Public Sub Greet()
    Print("Hello, my name is " + Name)
  End Sub
End Class

// -----------------------------------------------------------------------------
// Main Script Execution
// -----------------------------------------------------------------------------

// Define and use variables
Dim x As Integer = 5
Dim y As Integer = 10

// Output sum of two numbers
Print("Sum: " + Str(x + y))

// Conditional logic test
If x < y Then
  Print("x is less than y")
Else
  Print("x is not less than y")
End If

// For loop from 1 to 3
For i = 1 To 3
  Print("Loop: " + Str(i))
Next

// Color literal assignment and output
Dim purple As Color = &c800080
Print("Color literal for purple: " + Str(purple))

// Use of built-in math functions
Dim result As Double = Pow(2, 3) + Sqrt(16)  // 8 + 4 = 12
Print("Result: " + Str(result))

// Use Random class to generate a number between 1 and 100
Dim rndInstance As New Random
Print("Random Number (1 to 100): " + Str(rndInstance.InRange(1, 100)))

// Use module-defined function to multiply two integers
Print("Multiply 6 * 7 = " + Str(MathUtilities.Multiply(6, 7)))

// Create and use an instance of the Person class
Dim p As New Person("Alice")
p.Greet()               // Output greeting
