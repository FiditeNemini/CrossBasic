// Create a new Shell instance
Var shell As New Shell

// Define the command to execute 
Var command As String = "ls"

// Set a timeout of 5 seconds
shell.SetTimeout(5)

// Execute the command
If shell.Execute(command) Then
    // Retrieve and print the command output
    Print("Command Output: " + shell.Result)
    
    // Retrieve and print the exit code
    Print("Exit Code: " + Str(shell.ExitCode))
Else
    Print("Failed to execute command.")
End If

// Destroy the shell instance
shell.Close()
