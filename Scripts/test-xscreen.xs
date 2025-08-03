// -----------------------------------------------------------------------------
// Demo: Multi-Display Enumeration with XScreen in CrossBasic
// This script demonstrates how to query information about all connected displays
// using the `XScreen` class. It shows resolution and display descriptions.
// -----------------------------------------------------------------------------

// Create a new XScreen instance to access display information
Dim nx As New XScreen

// Get the last display index (usually DisplayCount - 1)
Dim idx As Integer = nx.DisplayCount - 1

// Access the primary (main) display
Dim disp As XScreen = nx.DisplayAt(0)  // Index 0 = main display

// Show resolution of the primary display
MessageBox("Main is " + Str(disp.Width) + "x" + Str(disp.Height))

// Loop through all displays and show their descriptions
For i As Integer = 0 To nx.LastDisplayIndex
  Dim d As XScreen = nx.DisplayAt(i)
  MessageBox("Display " + Str(i) + ": " + d.Description)
Next i
