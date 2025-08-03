// -----------------------------------------------------------------------------
// Demo: SQLiteDatabase Usage in CrossBasic
// This example demonstrates how to use the SQLiteDatabase and SQLiteStatement
// classes to create a database, execute SQL commands, insert records using
// prepared statements with parameter binding, and query data using a SELECT.
// -----------------------------------------------------------------------------

// Create a new SQLiteDatabase instance
Var db As New SQLiteDatabase

// Set the path of the SQLite database file and attempt to open it
db.DatabaseFile = "mydb.sqlite"
If db.Open() = False Then
    // If the database fails to open, print the error and exit
    Print("Failed to open database: " + db.LastError)
    Return 0
End If

// Create a new table named 'users' with 'id' and 'name' fields
db.ExecuteSQL("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)")
Print("Last Error: " + db.LastError)  // Print any error from table creation

// Create a new SQLiteStatement for inserting data
Var stmt As New SQLiteStatement
stmt.DatabaseHandle = db.Handle()   // Assign the database handle to the statement
stmt.SQL = "INSERT INTO users (name) VALUES (?)"  // Use parameter placeholder for safe insertion

// Prepare the INSERT statement
If stmt.Prepare() = False Then
    Print("Failed to prepare statement.")
    Return 0
End If

// Bind a string value ("John Doe") to the first placeholder and execute the statement
stmt.BindString(1, "John Doe")
stmt.Execute()
stmt.Finalize()  // Clean up after executing the insert

// Prepare a SELECT statement to retrieve all users
Var stmt As New SQLiteStatement
stmt.DatabaseHandle = db.Handle()
stmt.SQL = "SELECT id, name FROM users"

// Prepare and execute the SELECT statement
If stmt.Prepare() Then
    // Move to the first row, if any rows exist
    If stmt.MoveToFirstRow() Then
        // Loop through each row in the result set
        While stmt.MoveToNextRow() = True
            // Retrieve column values from current row
            Var id   As Integer = stmt.ColumnInteger(0)
            Var name As String  = stmt.ColumnString(1)

            // Print the row data
            Print("ID: " + Str(id) + ", Name: " + name)
        Wend
    End If
    stmt.Finalize()  // Finalize the SELECT statement
Else
    Print("Failed to prepare SELECT statement.")
End If

// Close the database to free up resources
db.Close()
