// -----------------------------------------------------------------------------
// Demo: DateTime Class Usage in CrossBasic
// This example demonstrates how to create, initialize, and extract components
// from a DateTime object. It shows formatting, accessing individual time parts,
// and properly cleaning up after use.
// -----------------------------------------------------------------------------

// Create a new DateTime instance to represent a specific moment in time
Dim dt As New DateTime

// Initialize the DateTime with a specific date and time:
// Parameters: year=2025, month=3 (March), day=17, hour=20, minute=8, second=27
dt.Initialize(2025, 3, 17, 20, 8, 27)

// Output the complete formatted DateTime string to the console
Print("Specific DateTime: " + dt.ToString)

// Extract and display individual components of the date and time
Print("Year: " + dt.GetYear().ToString)     // Outputs: 2025
Print("Month: " + dt.GetMonth().ToString)   // Outputs: 3 (March)
Print("Day: " + dt.GetDay().ToString)       // Outputs: 17
Print("Hour: " + dt.GetHour().ToString)     // Outputs: 20 (8 PM)
Print("Minute: " + dt.GetMinute().ToString) // Outputs: 8
Print("Second: " + dt.GetSecond().ToString) // Outputs: 27

// Properly clean up by destroying the DateTime instance
dt.Destroy()
