// XamlContainerDemo.cb
// A simple CrossBasic demo using the XamlContainer plugin:
// – Shows a window with a XAML Button loaded at runtime
// – Hooks the Button’s Click via AddXamlEvent

// Create the main window
Var win As New XWindow
win.Title            = "XAML Container Demo"
win.HasMaximizeButton = False
win.Resizable        = False
win.Width            = 600
win.Height           = 400
AddHandler(win.Closing, AddressOf(AppClosing))

Sub AppClosing()
  Quit()
End Sub

' // Create a label to describe the demo
' Var lbl As New XLabel
' lbl.Parent   = win.Handle
' lbl.Left     = 20
' lbl.Top      = 20
' lbl.Width    = 300
' lbl.Height   = 24
' lbl.Text     = "Below is a XAML‑hosted button:"
' lbl.FontSize = 14

// Create the XAML container
Var xc As New XamlContainer
xc.Parent = win.Handle
xc.Left   = 20
xc.Top    = 60
xc.Width  = win.Width - 40
xc.Height = win.Height - 100

// Load XAML with a named Button in the center
Var xamlMarkup As String = _
  "<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " + _
        "HorizontalAlignment='Stretch' VerticalAlignment='Stretch'>" + _
    "<Button x:Name='MyButton' Content='Press Me' Width='120' Height='40' " + _
            "HorizontalAlignment='Center' VerticalAlignment='Center'/>" + _
  "</Grid>"
xc.LoadXAML(xamlMarkup)

// Hook its Click dynamically
xc.AddXamlEvent("MyButton", "Click", AddressOf(OnMyBtnClick))

Sub OnMyBtnClick()
  MessageBox("You clicked the XAML button!")
End Sub

// Show the window and run the event loop
win.Show()

While True
  DoEvents()
  Sleep(10)
Wend
