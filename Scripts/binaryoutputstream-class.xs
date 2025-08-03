// -----------------------------------------------------------------------------
// Demo: BinaryOutputStream Usage in CrossBasic
// This code demonstrates how to use the BinaryOutputStream class to create
// and write binary data to a file. It shows how to write different data types
// (byte, short, long, double, string) and properly flush and close the stream.
// -----------------------------------------------------------------------------

// Create an instance of BinaryOutputStream to handle binary file writing
Var stream As New BinaryOutputStream

// Specify the target binary file path and disable append mode to overwrite the file
stream.FilePath = "example.bin"
stream.Append = False

// Attempt to open the binary file for writing
If stream.Open() = false Then
  // If the file couldn't be opened, display an error and exit
  Print("Failed to open file.")
  Return 0
End If

// Write a single byte (8 bits) with the value 255 to the file
stream.WriteByte(255)

// Write a 2-byte (16-bit) integer with the value 32000
stream.WriteShort(32000)

// Write a 4-byte (32-bit) integer with the value 123456789
stream.WriteLong(123456789)

// Write an 8-byte (64-bit) floating-point number representing Pi
stream.WriteDouble(3.14159)

// Write a string "Hello" to the binary file (typically stored as raw bytes)
stream.WriteString("Hello")

// Flush the stream to ensure all buffered data is written to disk
stream.Flush()

// Close the stream to release the file handle
stream.Close()

// Notify that the binary file has been written successfully
Print("Binary file written successfully.")
