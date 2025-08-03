////menu-demo.xs////

// -----------------------------------------------------------------------------
// Demo: Menu Bar and About Dialog in CrossBasic
// It shows how to create a menu bar with File and Help menus, respond to menu
// item selections, and perform actions like printing to the console or showing
// an About dialog.
// -----------------------------------------------------------------------------

// Create the main window
Var win As New XWindow
win.Width              = 400
win.Height             = 300
win.Title              = "Menu Demo"
win.Resizable          = False
win.HasCloseButton     = True
win.HasMinimizeButton  = True
win.HasMaximizeButton  = False
win.HasFullScreenButton = False
win.HasTitleBar        = True

AddHandler(win.Closing, AddressOf(AppClosing))

Sub AppClosing()
  Quit()
End Sub

// Create the menu bar
Var menuBar As New XMenuBar
menuBar.Parent = win.Handle

// ——— File Menu ———
Var fileMenu As New XMenuItem
fileMenu.Parent  = menuBar.Handle
fileMenu.Caption = "File"

  // New...
  Var newItem As New XMenuItem
  newItem.Parent  = fileMenu.Handle
  newItem.Caption = "New"
  AddHandler(newItem.Pressed, AddressOf(OnNew))

  // Open...
  Var openItem As New XMenuItem
  openItem.Parent  = fileMenu.Handle
  openItem.Caption = "Open"
  AddHandler(openItem.Pressed, AddressOf(OnOpen))

  // Separator
  Var sep1 As New XMenuItem
  sep1.Parent       = fileMenu.Handle
  sep1.IsSeparator  = True

  // Exit
  Var exitItem As New XMenuItem
  exitItem.Parent  = fileMenu.Handle
  exitItem.Caption = "Exit"
  AddHandler(exitItem.Pressed, AddressOf(OnExit))

// ——— Help Menu ———
Var helpMenu As New XMenuItem
helpMenu.Parent  = menuBar.Handle
helpMenu.Caption = "Help"

  // About
  Var aboutItem As New XMenuItem
  aboutItem.Parent  = helpMenu.Handle
  aboutItem.Caption = "About"
  AddHandler(aboutItem.Pressed, AddressOf(OnAbout))

// Show the window
win.Show()
While True
  DoEvents(10)
Wend

// ——— Event Handlers ———
Sub OnNew()
    MessageBox("Hello")
  Print("File -> New selected")
End Sub

Sub OnOpen()
  Print("File -> Open selected")
End Sub

Sub OnExit()
  Quit()
End Sub

Sub OnAbout()
  MessageBox("CrossBasic Menu Demo" + EndOfLine + "Version 1.0")
End Sub

