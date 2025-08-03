// -----------------------------------------------------------------------------
// Demo: BinaryInputStream Usage in CrossBasic
// This code demonstrates how to use the BinaryInputStream class to open,
// read, and process binary data from a file. It showcases reading various
// data types (byte, short, long, double, string) and outputs them to the console.
// -----------------------------------------------------------------------------

// Create an instance of BinaryInputStream to handle binary file operations
Var stream As New BinaryInputStream

// Specify the path of the binary file to read from
stream.FilePath = "example.bin"

// Attempt to open the binary file for reading
If stream.Open() = False Then
  // If the file couldn't be opened, output an error message and exit
  Print("Failed to open file.")
  Return 0
End If

// Read a single byte (8 bits) from the file
Var byteValue As Integer = stream.ReadByte()

// Read a 2-byte (16-bit) integer from the file
Var shortValue As Integer = stream.ReadShort()

// Read a 4-byte (32-bit) integer from the file
Var longValue As Integer = stream.ReadLong()

// Read an 8-byte (64-bit) floating-point number from the file
Var doubleValue As Double = stream.ReadDouble()

// Read 10 bytes from the file and convert them to a string
Var textData As String = stream.ReadString(10)

// Display the values read from the binary file to the console
Print("Byte: " + byteValue.ToString)
Print("Short: " + shortValue.ToString)
Print("Long: " + longValue.ToString)
Print("Double: " + doubleValue.ToString)
Print("Text: " + textData)

// Close the file stream to release the file handle
stream.Close()
