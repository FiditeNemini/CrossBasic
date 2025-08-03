////editor.xs////

// -----------------------------------------------------------------------------
// Demo: Simple Single-File Text Editor in CrossBasic
// - Creates a window with a menu bar (File → Save, Exit)
// - Auto-opens "editor.txt" if it exists on startup
// - Allows editing the text in an XTextArea and saving back to "editor.txt"
// -----------------------------------------------------------------------------

// Main window setup
Var win As New XWindow
win.Title              = "CrossBasic Text Editor"
win.Width              = 800
win.Height             = 600
win.HasCloseButton     = True
win.HasMinimizeButton  = True
win.HasMaximizeButton  = False
win.HasFullScreenButton = False
win.HasTitleBar        = True
win.Resizable          = True

AddHandler(win.Closing, AddressOf(OnExit))

// Menu bar (File → Save, Exit) :contentReference[oaicite:0]{index=0}:contentReference[oaicite:1]{index=1}
Var menuBar As New XMenuBar
menuBar.Parent = win.Handle

// ——— File Menu ———
Var fileMenu As New XMenuItem
fileMenu.Parent  = menuBar.Handle
fileMenu.Caption = "File"

  // Save… :contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3}
  Var saveItem As New XMenuItem
  saveItem.Parent  = fileMenu.Handle
  saveItem.Caption = "Save"
  AddHandler(saveItem.Pressed, AddressOf(OnSave))

  // Separator
  Var sep As New XMenuItem
  sep.Parent      = fileMenu.Handle
  sep.IsSeparator = True

  // Exit
  Var exitItem As New XMenuItem
  exitItem.Parent  = fileMenu.Handle
  exitItem.Caption = "Exit"
  AddHandler(exitItem.Pressed, AddressOf(OnExit))

// Text area for editing :contentReference[oaicite:4]{index=4}:contentReference[oaicite:5]{index=5}
Var editorArea As New XTextArea
editorArea.Parent = win.Handle
editorArea.Left   = 0
editorArea.Top    = 0    // leave room for the menu bar
editorArea.Width  = win.Width- 15
editorArea.Height = win.Height - (15 + 43)
editorArea.FontName = "Consolas"
editorArea.FontSize = 14
editorArea.TextColor = &cFFFFFF
editorArea.Visible   = True

AddHandler(editorArea.TextChanged, AddressOf(UpdateTextAreaPaint))

// Auto‑open file on startup :contentReference[oaicite:6]{index=6}:contentReference[oaicite:7]{index=7}
AddHandler(win.Opening, AddressOf(OnOpenFile))

// Show the window and enter main loop
win.Show()
While True
  DoEvents(200)
Wend

// ——— Event Handlers ———

// Load "editor.txt" into the text area
Sub OnOpenFile()
  Var inputStream As New TextInputStream
  inputStream.FilePath = "editor.txt"
  If inputStream.Open() = False Then
    // No existing file; start with empty editor
    editorArea.Text = ""
  End If

  // Read all lines and append to the editor
  editorArea.Text = ""
  While inputStream.EOF() = 0
    editorArea.Text = editorArea.Text + inputStream.ReadLine() + EndOfLine
  Wend
  inputStream.Close()
End Sub

// Save the contents of the text area back to "editor.txt"
Sub OnSave()
  Var outputStream As New TextOutputStream
  outputStream.FilePath = "editor.txt"
  outputStream.Append   = False
  If outputStream.Open() = False Then
    win.MessageBox("Failed to open file for saving.")
  End If
  Speak("File has been saved!")
  Print("Saved")
  // Write full contents without additional newlines
  outputStream.Write(editorArea.Text)
  return 0
End Sub

Sub UpdateTextAreaPaint()
  editorArea.Invalidate()
End Sub

// Exit the application
Sub OnExit()
  Quit()
  return 0
End Sub

////EOF////
