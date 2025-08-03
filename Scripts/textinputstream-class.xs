// -----------------------------------------------------------------------------
// Demo: TextInputStream – Reading Lines from a Text File in CrossBasic
// This script demonstrates how to open a text file using `TextInputStream`,
// read its contents line by line, and print each line to the console.
// -----------------------------------------------------------------------------

// Create a new instance of TextInputStream
Var inputStream As New TextInputStream

// Set the file path of the text file to read
inputStream.FilePath = "example.txt"

// Attempt to open the file for reading
If inputStream.Open() = False Then
  // If the file cannot be opened, show an error and exit
  Print("Failed to open file.")
  Return 0
End If

// While the end of file (EOF) hasn't been reached...
While inputStream.EOF() = 0
  // Read the next line from the file
  Var line As String = inputStream.ReadLine()
  
  // Print the line to the console
  Print(line)
Wend

// Close the file stream to release the handle
inputStream.Close()
