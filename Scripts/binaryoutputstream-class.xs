// Create an instance of BinaryOutputStream
Var stream As New BinaryOutputStream

// Set properties: file path and append mode (False means overwrite)
stream.FilePath = "example.bin"
stream.Append = False

// Open the binary file for writing
If stream.Open() = false Then
  Print("Failed to open file.")
  Return 0
End If

// Write binary data
stream.WriteByte(255)
stream.WriteShort(32000)
stream.WriteLong(123456789)
stream.WriteDouble(3.14159)
stream.WriteString("Hello")

// Flush and close the file stream
stream.Flush()
stream.Close()

Print("Binary file written successfully.")
