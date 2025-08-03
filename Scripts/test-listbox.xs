// Demo_AddRows.xojo
// A simple CrossBasic/XWindow demo:
// - Create a window
// - Place a 2‑column listbox and a button
// - On each button click, add a new row

// Create the main window
Var win As New XWindow
win.Width  = 600
win.Height = 800
win.Caption = "Listbox + Button Demo"

// Create the listbox
Var lb As New XListbox
lb.Width = 350
lb.height = 200
lb.left = 100
lb.top = 100
lb.ColumnCount    = 2
lb.ColumnWidths   = "50%,50%"
lb.RowHeight      = 20
lb.HasHeader      = True
lb.Visible        = True
lb.FontName       = "Arial"
lb.FontSize       = 16
'lb.Hasborder = true
var c as Color = &c0000FF
lb.TextColor      = c
lb.Parent         = win.Handle


// Set up two column headers
lb.AddRow("Item")   // use header row as simple workaround

// Create the button
Var btn As New XButton
btn.Width    = 80
btn.Height   = 30
btn.Left     = 10
btn.Top      = 10
btn.Parent   = win.Handle
btn.Caption  = "Add Row"


var txt As New XTextField
txt.Width = 200
txt.height = 30
txt.top = 10
txt.left = 100
txt.Parent = win.handle
txt.FontName = "Arial"
txt.FontSize = 20
txt.TextColor = &cFF0000
' // Row‑counter
 Dim rowCount As Integer = 1

' // Button callback: adds a new row "Row N" / "Value N"
Function OnAddRow()
  lb.AddRow("Row " + Str(rowCount))
  rowCount += 1 'rowCount + 1
  win.Title = str(lb.SelectedRow)
End Function

sub rowselected()
    txt.Text = lb.CellTextAt(lb.SelectedRow, 0)
end sub


Sub AppQuit()
  Quit()
End Sub

// Show the window
win.Show()

' // Wire up the button’s pressed event
 AddHandler(btn.Pressed, AddressOf(OnAddRow))
 AddHandler(lb.SelectionChanged, AddressOf(rowselected))
 AddHandler(win.closing, AddressOf(AppQuit))

// Event loop
While True
  DoEvents(1)
Wend
