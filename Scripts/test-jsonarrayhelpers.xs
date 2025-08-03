// -----------------------------------------------------------------------------
// Demo: JSONItem as an Array in CrossBasic
// This script demonstrates using a `JSONItem` to represent and manipulate
// a JSON array. It includes adding items, inserting at a specific index,
// iterating through values, and removing elements.
// -----------------------------------------------------------------------------

// Create a new JSONItem and treat it as an array
Var arr As New JSONItem

// Add items to the JSON array
arr.Add("Apple")
arr.Add("Banana")
arr.Add("Cherry")

// Insert "Blueberry" at index 1 (between Apple and Banana)
arr.AddAt(1, "Blueberry")

// Print the full array as JSON
Print("Array JSONItem: " + arr.ToString)

// Iterate through the JSON array and print each element
For i As Integer = 0 To arr.Count - 1
  Print("Element " + Str(i) + ": " + arr.ValueAt(i))
Next i

// Remove the element at index 2 (Banana gets removed)
arr.RemoveAt(2)

// Show the updated array
Print("After RemoveAt(2): " + arr.ToString)

// Clean up the JSONItem instance
arr.Close()
