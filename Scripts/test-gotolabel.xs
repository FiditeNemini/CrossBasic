// -----------------------------------------------------------------------------
// Demo: Simulating Meal Service with Nested Loops and Goto in CrossBasic
// This script iterates through a set of people and serves them meals across
// different times of day. It demonstrates string padding, formatting,
// arithmetic calculations, and use of Goto for custom loop control flow.
// -----------------------------------------------------------------------------

// Initialize counters and data arrays
Dim z As Integer = 0  // Outer loop counter
Dim arrPeople() As String = Array("Sally", "John", "Jane", "Robert", "Mitchel")
Dim totalPeople As Integer = arrPeople.Count()

Dim arrMeal() As String = Array("Breakfast", "Lunch", "Dinner")
Dim totalIterations As Integer = arrMeal.Count()

Dim mealTimeofDay As Integer = -1  // Will start at 0 on first loop
Dim costPerMeal As Double = 6.93
Dim totalMeals As Integer  // Counter for total meals served

// Function: Returns a pipe character divider between columns
Function Divider() As String
  Return " | "
End Function

// Function: Pads a string on the right to ensure fixed-width columns
Function PadRight(text As String, width As Integer) As String
  If Length(text) < width Then
    Return text + Space(width - Length(text))
  Else
    Return text
  End If
End Function

// Function: Computes meal number based on outer and inner loop counters
Function ServeAMeal(mainIt As Integer, loopIt As Integer) As Integer
  Return mainIt * totalPeople + loopIt
End Function

// Main program logic using Goto for loop simulation
Beginning:
  mealTimeofDay = mealTimeofDay + 1  // Increment meal time index

  // Serve a meal to each person in the current time of day
  For I As Integer = 1 To totalPeople
    totalMeals = ServeAMeal(z, I)  // Compute total meals served so far

    // Print a formatted row of meal data
    Print( _
      PadRight("Meal - " + arrMeal(mealTimeofDay), 20) + Divider() + _
      PadRight(arrPeople(I - 1), 15) + Divider() + _
      "Total Meals Served = " + Str(totalMeals) _
    )
  Next I

  z = z + 1
  If z = totalIterations Then
    Goto Done  // Exit loop if all meals are served
  End If

  Goto Beginning  // Repeat for next meal time

// Final summary output
Done:
  Print("")
  Print("Total Meals Served = " + Str(totalMeals) + _
        " at $" + Format(costPerMeal, "#0.00") + _
        " for a total of: $" + Format(costPerMeal * totalMeals, "#0.00"))
  Print("All Done!")
