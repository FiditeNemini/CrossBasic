// Create a new instance of TextInputStream
Var inputStream As New TextInputStream

// Set the file path property
inputStream.FilePath = "example.txt"

// Open the text file for reading
If inputStream.Open() = False Then
  Print("Failed to open file.")
  Return 0
End If

// Read and display each line until EOF is reached
While inputStream.EOF() = 0
  Var line As String = inputStream.ReadLine()
  Print(line)
Wend

// Close the file stream
inputStream.Close()
