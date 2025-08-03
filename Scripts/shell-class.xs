// -----------------------------------------------------------------------------
// Demo: Shell Command Execution in CrossBasic
// This example demonstrates how to execute a shell command using the Shell class,
// retrieve its output and exit code, and handle potential execution failures.
// -----------------------------------------------------------------------------

// Create a new Shell instance to run system commands
Var shell As New Shell

// Define the shell command to execute (e.g., list directory contents)
Var command As String = "ls"

// Set the execution timeout to 5 seconds (prevents hanging processes)
shell.SetTimeout(5)

// Execute the command using the shell
If shell.Execute(command) Then
    // If execution succeeds, print the command's standard output
    Print("Command Output: " + shell.Result)

    // Also print the numeric exit code returned by the command
    Print("Exit Code: " + Str(shell.ExitCode))
Else
    // If execution fails, display an error message
    Print("Failed to execute command.")
End If

// Clean up the Shell instance
shell.Close()
