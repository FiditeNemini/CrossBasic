// -----------------------------------------------------------------------------
// Demo: Reverse Text Input in CrossBasic
// This script prompts the user to enter text. It reverses the input and prints it.
// Typing `exit()` exits the program gracefully.
// -----------------------------------------------------------------------------

// Declare user input variable
Var userInput As String

// Prompt user
Print("Enter some text, press 'Enter.' Input 'exit()' to quit")

// Loop until user types 'exit()'
While userInput <> "exit()"

  // Get user input from console
  userInput = Input()

  // Exit condition check
  If userInput = "exit()" Then 
    Return 0
  End If

  // Split input into character array
  Var y() As String = Split(userInput, "")

  // Declare a variable to hold reversed string
  Var t As String

  // Reverse the characters in the array and concatenate
  For i As Integer = y.LastIndex() DownTo 0
    t = t + y(i)
  Next

  // Output reversed string
  Print("Your text backwards: " + t)

Wend
