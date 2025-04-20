// Create a new DateTime instance
Dim dt As New DateTime

// Initialize the instance with a specific date and time:
// (Parameters: year, month, day, hour, minute, second)
dt.Initialize(2025, 3, 17, 20, 8, 27)

// Output the formatted date and time
Print("Specific DateTime: " + dt.ToString)

// Retrieve individual components
Print("Year: " + dt.GetYear().ToString)
Print("Month: " + dt.GetMonth().ToString)
Print("Day: " + dt.GetDay().ToString)
Print("Hour: " + dt.GetHour().ToString)
Print("Minute: " + dt.GetMinute().ToString)
Print("Second: " + dt.GetSecond().ToString)

// Clean up by destroying the DateTime instance
dt.Destroy()
