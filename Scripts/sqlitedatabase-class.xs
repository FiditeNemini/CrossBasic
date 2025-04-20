// Create a new SQLiteDatabase instance
Var db As New SQLiteDatabase

// Set the database file and open the database.
db.DatabaseFile = "mydb.sqlite"
If db.Open() = False Then
    Print("Failed to open database: " + db.LastError)
    Return 0
End If


// Execute a SQL command to create a table.
db.ExecuteSQL("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)")
Print("Last Error: " + db.LastError)

// Set properties for the statement.
var stmt as new SQLiteStatement
stmt.DatabaseHandle = db.Handle()   // db is the integer handle of the SQLiteDatabase instance.
stmt.SQL = "INSERT INTO users (name) VALUES (?)"

// Prepare the statement.
If stmt.Prepare() = False Then
    Print("Failed to prepare statement.")
    Return 0
End If

// Bind a value and execute the statement.
stmt.BindString(1, "John Doe")
stmt.Execute()
stmt.Finalize()

// Now prepare a SELECT statement.
var stmt as new SQLiteStatement
stmt.DatabaseHandle = db.Handle()
stmt.SQL = "SELECT id, name FROM users"
If stmt.Prepare() Then
    If stmt.MoveToFirstRow() Then
        While stmt.MoveToNextRow() = True
            Var id As Integer = stmt.ColumnInteger(0)
            Var name As String = stmt.ColumnString(1)
            Print("ID: " + Str(id) + ", Name: " + name)
        Wend
    End If
    stmt.Finalize()
Else
    Print("Failed to prepare SELECT statement.")
End If


// Close the database.
db.Close()