// Create a new JSONItem instance
Var jItem As New JSONItem

// Set some values (acting like a dictionary).
jItem.SetValue("Name", "John Doe")
jItem.SetValue("Age", "32")
jItem.SetValue("Married", "true")
jItem.SetValue("Spouse", "Jane Doe")

// Create a new JSONItem for an array of kids.
Var kids As New JSONItem
kids.Add("Alice")
kids.Add("Bob")
kids.Add("Charlie")

// Set the Kids property of the person object.
jItem.SetValue("Kids", kids.ToString)

// Optionally, set formatting properties.
jItem.Compact = False      ' Produce pretty printed JSON.
jItem.IndentSpacing = 4    ' Use 4 spaces per indent.

// Get the JSON string.
Var jsonString As String = jItem.ToString
Print "JSONItem ToString output:"
Print jsonString

// Display some properties.
Print "Count: " + Str(jItem.Count())
If jItem.HasKey("Name") Then
  Print "Name key exists."
Else
  Print "Name key missing."
End If

// Lookup a value with a default.
Var ageValue As String = jItem.Lookup("Age", "Not Found")
Print("Age: " + ageValue)

// Get keys (returns a JSON array string of keys).
Var keysString As String = jItem.Keys()
Print("Keys: " + keysString)

// Remove a key and display updated JSON.
jItem.Remove("Married")
Print("After removing 'Married': " + jItem.ToString)

// Finally, destroy the JSONItem instance.
jItem.Close()



// --- JSONItem Array Demo ---

// Create a new JSONItem instance to be used as an array.
Var arr As New JSONItem

// Add several elements to the array.
arr.Add("Apple")
arr.Add("Banana")
arr.Add("Cherry")

// Insert an element at index 1.
arr.AddAt(1, "Blueberry")

// Output the JSON representation of the array.
Print("Array JSONItem (ToString): " + arr.ToString)

// Loop through the array.
// (Assuming the plugin exposes a method ValueAt(index) to return the element at that index.)
For i As Integer = 0 To arr.Count() - 1
    // Retrieve the element at index i.
    Var element As String = arr.ValueAt(i)
    Print("Element " + Str(i) + ": " + element)
Next i

// (Optional) If needed, remove an element and display the updated array.
arr.RemoveAt(2) // Removes the element at index 2.
Print("After RemoveAt(2): " + arr.ToString)

// Finally, destroy the JSONItem instance.
arr.Close()
