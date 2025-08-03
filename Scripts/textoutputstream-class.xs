// -----------------------------------------------------------------------------
// Demo: TextOutputStream – Writing to a Text File in CrossBasic
// This script demonstrates how to create or overwrite a text file using
// `TextOutputStream`, write data with and without line breaks, and ensure
// data is saved properly using `Flush`.
// -----------------------------------------------------------------------------

// Create a new instance of TextOutputStream for file writing
Var stream As New TextOutputStream

// Set the path to the output file and choose not to append (overwrite instead)
stream.FilePath = "example.txt"
stream.Append = False

// Attempt to open the file for writing
If stream.Open() = False Then
  // Exit if file could not be opened
  Print("Failed to open file.")
  Return 0
End If

// Write lines of text (adds a newline after each)
stream.WriteLine("Hello, world!")
stream.WriteLine("This is a test.")

// Write more text without adding newlines
stream.Write("More text without newline.")
stream.Write(" Even More text without newline.")

// Flush the buffer to ensure all data is written 
// (Good practice with large amounts of data being written.)
stream.Flush()

// Close the stream to release the file handle
stream.Close()

// Indicate completion
Print("Text file written successfully.")
