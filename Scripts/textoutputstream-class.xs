// Create a new instance of TextOutputStream
Var stream As New TextOutputStream

// Set properties: file path and append mode (False means overwrite)
stream.FilePath = "example.txt"
stream.Append = False

// Open the file for writing
If stream.Open() = false Then
  Print("Failed to open file.")
  Return 0
End If

// Write text data
stream.WriteLine("Hello, world!")
stream.WriteLine("This is a test.")
stream.Write("More text without newline.")
stream.Write(" Even More text without newline.")

// Flush and close the file
stream.Flush()
stream.Close()

Print("Text file written successfully.")
