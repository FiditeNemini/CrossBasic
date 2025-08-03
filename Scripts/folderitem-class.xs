// -----------------------------------------------------------------------------
// Demo: FolderItem File and Directory Operations in CrossBasic
// This example demonstrates how to use the FolderItem class to interact with
// the filesystem. It includes checking for file existence, creating directories,
// retrieving and setting permissions, and deleting files.
// -----------------------------------------------------------------------------

// Create a FolderItem instance representing a file
Dim fileItem As New FolderItem
fileItem.Path = "testfile.txt"

// Check if the specified file exists on disk
If fileItem.Exists() Then
    Print("File exists!")
Else
    Print("File does not exist.")
End If

// Create a FolderItem instance representing a directory
Dim dirItem As New FolderItem
dirItem.Path = "./TestFolder"

// Attempt to create a new directory at the specified path
If dirItem.CreateDirectory() Then
    Print("Directory created successfully!")
Else
    Print("Failed to create directory.")
End If

// Retrieve the size of the file in bytes
Dim fileSize As Integer = fileItem.Size()
Print("File size: " + Str(fileSize) + " bytes")

// Determine if the FolderItem represents a directory or a regular file
If fileItem.IsDirectory() Then
    Print("This is a directory.")
Else
    Print("This is a file.")
End If

// Retrieve the full system path of the file
Dim fullPath As String = fileItem.GetPath()
Print("Full path: " + fullPath)

// Retrieve the file's current Unix-style permission value (e.g., 644)
Dim permissions As Integer = fileItem.GetPermission()
Print("File permissions: " + Str(permissions))

// Set new permissions for the file (only works on Unix-like systems)
Dim newPermissions As Integer = 644
If fileItem.SetPermission(newPermissions) Then
    Print("Permissions updated successfully!")
Else
    Print("Failed to change permissions.")
End If

// Retrieve the URL-safe version of the file path
Dim urlPath As String = fileItem.URLPath()
Print("URL Path: " + urlPath)

// Retrieve a shell-safe version of the file path (for use in scripts/commands)
Print("Shell Path: " + fileItem.ShellPath())

// Attempt to delete the file
If fileItem.Delete() Then
    Print("File deleted successfully!")
Else
    Print("Failed to delete file.")
End If

// Close the FolderItem instances to release system handles
fileItem.Close()
dirItem.Close()
