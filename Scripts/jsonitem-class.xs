// -----------------------------------------------------------------------------
// Demo: JSONItem Usage in CrossBasic
// This example demonstrates creating and manipulating a JSON structure using
// the JSONItem class. It shows how to set values, nest arrays/objects, format
// output, check keys, retrieve values, and remove elements.
// -----------------------------------------------------------------------------


// Create a new JSONItem instance to hold structured data
Var jItem As New JSONItem

// Set key-value pairs for basic properties in the JSON object

jItem.SetValue("Name",    "John Doe")
jItem.SetValue("Age",     "32")
jItem.SetValue("Married", "true")
jItem.SetValue("Spouse",  "Jane Doe")


// Create a new JSONItem instance to hold an array of children
Var kids As New JSONItem
kids.Add("Alice")
kids.Add("Bob")
kids.Add("Charlie")

// Add the 'kids' array as a child object of the main JSON structure
jItem.SetValue("Kids", kids.toString) 


// Configure the JSON output: human-readable with indentation
jItem.Compact       = False        // Disable compact formatting
jItem.IndentSpacing = 4           // Use 4-space indentation

// Display the full formatted JSON string
Print("JSONItem ToString output:")
Print(jItem.toString)

// Print the number of top-level keys in the JSON object
Print("Count: " + Str(jItem.Count))

// Check for the existence of the "Name" key and output result
If jItem.HasKey("Name") Then
  Print("Name key exists.")
Else
  Print("Name key missing.")
End If

// Safely retrieve the value of the "Age" key, or return "Not Found" if missing
Print("Age: " + jItem.Lookup("Age", "Not Found")) //Returns a plain string
Print("Age: " + jItem.value("Age")) //Returns JSONItem converted to a String by Print()
Print("Name: " + jItem.value("Name")) //Returns JSONItem converted to a String by Print()

// Print a list of all keys in the JSON object
Print("Keys: " + jItem.Keys()) //JSONItem Array as String by Print()

// Remove the "Married" key from the object
jItem.Remove("Married")
Print("After removing 'Married': " + jItem.ToString)

// Release resources for both JSONItem instances
jItem.Close()
kids.Close()
