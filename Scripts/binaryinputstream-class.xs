// Create an instance of BinaryInputStream
Var stream As New BinaryInputStream

// Set the file path property
stream.FilePath = "example.bin"

// Open the binary file for reading
If stream.Open() = False Then
  Print("Failed to open file.")
  Return 0
End If

// Read data from the file
Var byteValue As Integer = stream.ReadByte()
Var shortValue As Integer = stream.ReadShort()
Var longValue As Integer = stream.ReadLong()
Var doubleValue As Double = stream.ReadDouble()
Var textData As String = stream.ReadString(10) ' Read 10 bytes as a string

// Display results
Print("Byte: " + byteValue.ToString)
Print("Short: " + shortValue.ToString)
Print("Long: " + longValue.ToString)
Print("Double: " + doubleValue.ToString)
Print("Text: " + textData)

// Close the file stream
stream.Close()
