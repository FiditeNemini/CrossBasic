// CrossBasic CalculatorDemo

// Create the main window
Var win As New XWindow
win.Width   = 257
win.Height  = 387
win.HasMaximizeButton = False
Win.Resizable = False
win.Title = "Calculator"

// Create the display text field
Var txt As New XTextField
txt.Left      = 10
txt.Top       = 10
txt.Width     = 230
txt.Height    = 40
txt.Parent    = win.Handle
txt.FontName  = "Arial"
txt.FontSize  = 32
txt.Bold = True
txt.TextColor = &cFFFF00
txt.Text      = "0"

// Calculator state
Var operand1   As Double  = 0
Var operand2   As Double  = 0
Var currentOp  As String  = ""
Var isNewEntry As Boolean = True

// Layout parameters
Dim btnSize  As Integer = 50
Dim spacing  As Integer = 10
Dim startX   As Integer = 10
Dim startY   As Integer = txt.Top + txt.Height + spacing

// ——— Handlers ———

Sub OnNumber(label As String)
  If isNewEntry Then
    txt.Text      = label
    isNewEntry    = False
  Else
    txt.Text      = txt.Text + label
  End If
End Sub

Sub Zero()
    OnNumber("0")
End Sub
Sub One()
    OnNumber("1")
End Sub
Sub Two()
    OnNumber("2")
End Sub
Sub Three()
    OnNumber("3")
End Sub
Sub Four()
    OnNumber("4")
End Sub
Sub Five()
    OnNumber("5")
End Sub
Sub Six()
    OnNumber("6")
End Sub
Sub Seven()
    OnNumber("7")
End Sub
Sub Eight()
    OnNumber("8")
End Sub
Sub Nine()
    OnNumber("9")
End Sub

Sub Plus()
    OnOperator("+")
End Sub

Sub Minus()
    OnOperator("-")
End Sub

Sub Mult()
    OnOperator("*")
End Sub

Sub Div()
    OnOperator("/")
End Sub

Sub OnOperator(op As String)
  operand1     = Val(txt.Text)
  currentOp    = op
  isNewEntry   = True
End Sub

Sub OnEqual()
  operand2     = Val(txt.Text)
  Var result As Double
  Select Case currentOp
    Case "+"
        result = operand1 + operand2
    Case "-"
        result = operand1 - operand2
    Case "*"
        result = operand1 * operand2
    Case "/" 
      If operand2 <> 0 Then 
        result = operand1 / operand2 
      Else 
        result = 0
      End If
    Case Else
        result = Val(txt.Text)
  End Select
  txt.Text      = Str(result)
  isNewEntry    = True
End Sub

Sub OnC()
    txt.Text      = "0"
    operand1      = 0
    operand2      = 0
    currentOp     = ""
    isNewEntry    = True
End Sub

Sub OnCE()
    txt.Text      = "0"
    isNewEntry    = True
End Sub

// ——— Create buttons ———

Dim x As Integer
Dim y As Integer
Dim ClearType as String

// Row 1: C, CE, /, *
y = startY
x = startX
Var btnC As New XButton
btnC.Left      = x
btnC.Top       = y
btnC.Width     = btnSize
btnC.Height    = btnSize
btnC.Parent    = win.Handle
btnC.Caption   = "C"
AddHandler(btnC.Pressed, AddressOf(OnC))

x = x + btnSize + spacing
Var btnCE As New XButton
btnCE.Left     = x
btnCE.Top      = y
btnCE.Width    = btnSize
btnCE.Height   = btnSize
btnCE.Parent   = win.Handle
btnCE.Caption  = "CE"

AddHandler(btnCE.Pressed, AddressOf(OnCE))

x = x + btnSize + spacing
Var btnDiv As New XButton
btnDiv.Left    = x
btnDiv.Top     = y
btnDiv.Width   = btnSize
btnDiv.Height  = btnSize
btnDiv.Parent  = win.Handle
btnDiv.Caption = "/"

AddHandler(btnDiv.Pressed, AddressOf(Div))

x = x + btnSize + spacing
Var btnMul As New XButton
btnMul.Left    = x
btnMul.Top     = y
btnMul.Width   = btnSize
btnMul.Height  = btnSize
btnMul.Parent  = win.Handle
btnMul.Caption = "*"

AddHandler(btnMul.Pressed, AddressOf(Mult))

// Row 2: 7, 8, 9, –
y = y + btnSize + spacing
x = startX
Var btn7 As New XButton
btn7.Left      = x
btn7.Top       = y
btn7.Width     = btnSize
btn7.Height    = btnSize
btn7.Parent    = win.Handle
btn7.Caption   = "7"

AddHandler(btn7.Pressed, AddressOf(Seven))

x = x + btnSize + spacing
Var btn8 As New XButton
btn8.Left      = x
btn8.Top       = y
btn8.Width     = btnSize
btn8.Height    = btnSize
btn8.Parent    = win.Handle
btn8.Caption   = "8"

AddHandler(btn8.Pressed, AddressOf(Eight))

x = x + btnSize + spacing
Var btn9 As New XButton
btn9.Left      = x
btn9.Top       = y
btn9.Width     = btnSize
btn9.Height    = btnSize
btn9.Parent    = win.Handle
btn9.Caption   = "9"

AddHandler(btn9.Pressed, AddressOf(Nine))

x = x + btnSize + spacing
Var btnMinus As New XButton
btnMinus.Left     = x
btnMinus.Top      = y
btnMinus.Width    = btnSize
btnMinus.Height   = btnSize
btnMinus.Parent   = win.Handle
btnMinus.Caption  = "-"

AddHandler(btnMinus.Pressed, AddressOf(Minus))

// Row 3: 4, 5, 6, +
y = y + btnSize + spacing
x = startX
Var btn4 As New XButton
btn4.Left      = x
btn4.Top       = y
btn4.Width     = btnSize
btn4.Height    = btnSize
btn4.Parent    = win.Handle
btn4.Caption   = "4"

AddHandler(btn4.Pressed, AddressOf(Four))

x = x + btnSize + spacing
Var btn5 As New XButton
btn5.Left      = x
btn5.Top       = y
btn5.Width     = btnSize
btn5.Height    = btnSize
btn5.Parent    = win.Handle
btn5.Caption   = "5"

AddHandler(btn5.Pressed, AddressOf(Five))

x = x + btnSize + spacing
Var btn6 As New XButton
btn6.Left      = x
btn6.Top       = y
btn6.Width     = btnSize
btn6.Height    = btnSize
btn6.Parent    = win.Handle
btn6.Caption   = "6"

AddHandler(btn6.Pressed, AddressOf(Six))

x = x + btnSize + spacing
Var btnPlus As New XButton
btnPlus.Left     = x
btnPlus.Top      = y
btnPlus.Width    = btnSize
btnPlus.Height   = btnSize
btnPlus.Parent   = win.Handle
btnPlus.Caption  = "+"

AddHandler(btnPlus.Pressed, AddressOf(Plus))

// Row 4: 1, 2, 3, =
y = y + btnSize + spacing
x = startX
Var btn1 As New XButton
btn1.Left      = x
btn1.Top       = y
btn1.Width     = btnSize
btn1.Height    = btnSize
btn1.Parent    = win.Handle
btn1.Caption   = "1"

AddHandler(btn1.Pressed, AddressOf(One))

x = x + btnSize + spacing
Var btn2 As New XButton
btn2.Left      = x
btn2.Top       = y
btn2.Width     = btnSize
btn2.Height    = btnSize
btn2.Parent    = win.Handle
btn2.Caption   = "2"

AddHandler(btn2.Pressed, AddressOf(Two))

x = x + btnSize + spacing
Var btn3 As New XButton
btn3.Left      = x
btn3.Top       = y
btn3.Width     = btnSize
btn3.Height    = btnSize
btn3.Parent    = win.Handle
btn3.Caption   = "3"

AddHandler(btn3.Pressed, AddressOf(Three))

x = x + btnSize + spacing
Var btnEqual As New XButton
btnEqual.Left     = x
btnEqual.Top      = y
btnEqual.Width    = btnSize
btnEqual.Height   = btnSize
btnEqual.Parent   = win.Handle
btnEqual.Caption  = "="

AddHandler(btnEqual.Pressed, AddressOf(OnEqual))

// Row 5: 0
y = y + btnSize + spacing
x = startX
Var btn0 As New XButton
btn0.Left      = x
btn0.Top       = y
btn0.Width     = btnSize
btn0.Height    = btnSize
btn0.Parent    = win.Handle
btn0.Caption   = "0"

AddHandler(btn0.Pressed, AddressOf(Zero))

// Quit handler
Sub AppQuit()
  Quit()
End Sub
AddHandler(win.closing, AddressOf(AppQuit))

// Show window and start event loop
win.Show()
While True
  DoEvents(100)
Wend
