// Create an instance for file operations
Dim fileItem As New FolderItem
fileItem.Path = "testfile.txt"

// Check if the file exists
If fileItem.Exists() Then
    Print("File exists!")
Else
    Print("File does not exist.")
End If

// Create an instance for directory operations
Dim dirItem As New FolderItem
dirItem.Path = "./TestFolder"

// Create a new directory
If dirItem.CreateDirectory() Then
    Print("Directory created successfully!")
Else
    Print("Failed to create directory.")
End If

// Get file size
Dim fileSize As Integer = fileItem.Size()
Print("File size: " + Str(fileSize) + " bytes")

// Check if it is a directory
If fileItem.IsDirectory() Then
    Print("This is a directory.")
Else
    Print("This is a file.")
End If

// Get the full path of the file
Dim fullPath As String = fileItem.GetPath()
Print("Full path: " + fullPath)

// Get file permissions (Unix-style)
Dim permissions As Integer = fileItem.GetPermission()
Print("File permissions: " + Str(permissions))

// Set new file permissions (only works on Unix-based systems)
Dim newPermissions As Integer = 644
If fileItem.SetPermission(newPermissions) Then
    Print("Permissions updated successfully!")
Else
    Print("Failed to change permissions.")
End If

// Get the URL version of the file path
Dim urlPath As String = fileItem.URLPath()
Print("URL Path: " + urlPath)

// Get the shell-safe version of the file path
Print("Shell Path: " + fileItem.ShellPath())

// Delete the file
If fileItem.Delete() Then
    Print("File deleted successfully!")
Else
    Print("Failed to delete file.")
End If

fileItem.Close()
dirItem.Close()
