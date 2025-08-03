// -----------------------------------------------------------------------------
// Demo: Overloaded Constructors and Variant Arrays in CrossBasic
// This script demonstrates class overloading, instance property manipulation,
// and storing class instances inside Variant arrays for polymorphic handling.
// -----------------------------------------------------------------------------

// Define a class representing a pair of values
Class Pair
  // Properties for left and right values
  Dim LeftValue As Variant
  Dim RightValue As Variant

  // Default constructor (does nothing by default)
  Sub Constructor()
    // Main Constructor
  End Sub

  // Overloaded constructor to initialize left and right values directly
  Sub Constructor(leftValue As Variant, rightValue As Variant)
    Self.LeftValue = leftValue
    Self.RightValue = rightValue
  End Sub
End Class

// Create a Pair instance and assign values manually
Var p As New Pair
p.LeftValue = "hello "
p.RightValue = "world."
Print(p.LeftValue + p.RightValue)  // Output: hello world.

// Create another Pair using the overloaded constructor
Var c As New Pair
c.Constructor(30, 40)
Print(Str(c.LeftValue))   // Output: 30
Print(Str(c.RightValue))  // Output: 40

// Store both Pair instances in a Variant array
Var vpairs() As Variant
vpairs.Add(p)
vpairs.Add(c)

// Access properties through the Variant array (auto-unboxing)
Print(vpairs(0).LeftValue)  // Output: hello 
