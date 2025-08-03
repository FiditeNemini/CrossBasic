// -----------------------------------------------------------------------------
// Demo: GUI Application with Buttons, Timer, TextArea, and Beep in CrossBasic
// This example showcases creation of a basic GUI window using CrossBasic’s native
// `XWindow`, `XButton`, `XTextArea`, and `XTextField` controls. It handles events
// like button presses, window lifecycle, timer updates, and input text changes.
// -----------------------------------------------------------------------------

// Define some parameters
Dim TotalCount As Integer = 0
Dim red As Color = &cFF0000    // Red
Dim green As Color = &c00FF00  // Green
Dim white As Color = &cFFFFFF  // White
Dim blue As Color = &c0000FF   // Blue

// Get screen information and main display
Dim src As New XScreen
Dim disp As XScreen = src.DisplayAt(0)  // Main display reference

// Create main window
Dim win As New XWindow
win.ViewType = 0
win.Title = "My First GUI Application"
win.Height = 500
win.Width = 515
win.BackgroundColor = &c0B0B0B
win.Resizable = true
win.HasCloseButton = True
win.HasMinimizeButton = True
win.HasMaximizeButton = False

// Center window on screen
win.Left = (disp.Width / 2) - (win.Width / 2)
win.Top = (disp.Height / 2) - (win.Height / 2)

// Create and configure buttons
Dim btn As New XButton
// Assign parent window for controls
btn.Parent = win.Handle
btn.Left = 50
btn.Top = 50
btn.Width = 100
btn.Height = 30

Dim btn2 As New XButton
// Assign parent window for controls
btn2.Parent = win.Handle
btn2.Left = 170
btn2.Top = 50
btn2.Width = 200
btn2.Height = 30

// Create and configure text field
Dim tField1 as New XTextField
// Assign parent window for controls
tField1.Parent = win.Handle
tField1.Left = 50
tField1.Top = 10
tField1.Height = 30
tField1.Width = 400

// Create and configure text area
Dim txtArea1 as New XTextArea
// Assign parent window for controls
txtArea1.Parent = win.Handle
txtArea1.Left = 50
txtArea1.Top = 150
txtArea1.Height = 250
txtArea1.Width = 400
txtArea1.locktop = true
txtArea1.lockbottom = True
txtArea1.lockleft = true
txtArea1.lockright = true

' // Assign parent window for controls
' btn.Parent = win.Handle
' btn2.Parent = win.Handle
' txtArea1.Parent = win.Handle
' tField1.Parent = win.Handle

// Visual Properties must be set once attached to the parent
btn.Caption = "🙂 Click Me"
btn2.Caption = "Quit Application"
btn.visible = True
btn2.visible = True

btn.FontName = "Calibri"
btn.FontSize = 16
btn.HasBorder = True

btn2.FontName = "Arial"
btn2.FontSize = 12
btn2.Bold = True
btn2.Italic = True
btn2.Underline = True
btn2.HasBorder = True

tField1.FontName = "Calibri"
tField1.FontSize = 16
tField1.Bold = True
tField1.TextColor = Green
tField1.visible = True

txtArea1.FontName = "Calibri"
txtArea1.FontSize = 18
txtArea1.TextColor = white
txtArea1.visible = True


// Register event handlers
AddHandler(win.Opening, AddressOf(WindowOpening))
AddHandler(win.Closing, AddressOf(WindowClosed))
AddHandler(btn.Pressed, AddressOf(ButtonPressed))
AddHandler(btn2.Pressed, AddressOf(ButtonPressed2))
AddHandler(tField1.TextChanged, AddressOf(TField1TextChanged))


// Create and configure a repeating timer
Dim tmr As New XTimer
tmr.Period = 1        // 1 ms interval
tmr.RunMode = 2       // Multiple firing
AddHandler(tmr.Action, AddressOf(TimerFired))

// Show the window
win.Show()

// Main event loop
While True
  DoEvents(1)
Wend

// Event: Window opening
Sub WindowOpening()
  MessageBox("Welcome!")
End Sub

// Event: Window closed
Sub WindowClosed()
  MessageBox("Window was closed! Goodbye cruel world!")
  Quit()
End Sub

// Event: First button pressed (toggle timer and button state)
Sub ButtonPressed()
  If btn2.Enabled Then
    tmr.Enabled = False
    btn2.Enabled = False
    txtArea1.TextColor = Red
    txtArea1.Text = "Quit Button Disabled"
  Else
    btn2.Enabled = True
    tmr.Enabled = True
    txtArea1.TextColor = Green
    txtArea1.Text = "Quit Button Enabled"
  End If
End Sub

// Event: Second button pressed (quit app)
Sub ButtonPressed2()
  Quit()
End Sub

// Event: Timer — update window title and log counter
Sub TimerFired()
  win.Title = "Timer fired at " + GetCurrentTime() + " TotalCount: " + Str(TotalCount)
  TotalCount = TotalCount + 1
  txtArea1.AddText(Str(TotalCount) + EndOfLine)
  txtArea1.ScrollPosition = 999999
End Sub

// Event: Text field content changed — reflect and optionally beep
Sub TField1TextChanged(txtData As String)
  txtArea1.TextColor = White
  txtArea1.Text = txtData
  Dim freq As String = txtArea1.Text
  If IsNumeric(freq) Then
    Beep(Val(freq), 200)
  End If
End Sub
