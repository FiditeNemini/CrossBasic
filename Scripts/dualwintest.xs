Var NewView As New XWindow
NewView.Width = 375
NewView.Height = 667
NewView.BackgroundColor = &c1a1a1a
NewView.ViewType = 0
NewView.HasCloseButton = True
NewView.HasMinimizeButton = True
NewView.HasMaximizeButton = True
NewView.HasFullScreenButton = False
NewView.HasTitleBar = True
NewView.Resizable = True

AddHandler(NewView.Closing, AddressOf(NewView_Closing))

Sub NewView_Closing()
	Quit()
End Sub

Var Button As New XButton
Button.Parent = NewView.Handle
Button.Caption = "Button"
Button.TextColor = &c000000
Button.Bold = False
Button.Underline = False
Button.Italic = False
Button.FontName = "Arial"
Button.FontSize = 12
Button.Enabled = True
Button.Visible = True

AddHandler(Button.Pressed, AddressOf(Button_Pressed))

Sub Button_Pressed()
	'NewView2.ShowModal(NewView.Handle)
	NewView2.Show()
End Sub

Var Username As String = "Matthew"

// Show the window //////
NewView.Show()




Var NewView2 As New XWindow
NewView2.Width = 375
NewView2.Height = 375
NewView2.BackgroundColor = &cd2d2d2
NewView2.ViewType = 4
NewView2.HasCloseButton = True
NewView2.HasMinimizeButton = True
NewView2.HasMaximizeButton = True
NewView2.HasFullScreenButton = False
NewView2.HasTitleBar = False
NewView2.Resizable = False


Var Button2 As New XButton
Button2.Parent = NewView2.Handle
Button2.Caption = "Button"
Button2.TextColor = &c000000
Button2.Bold = False
Button2.Underline = False
Button2.Italic = False
Button2.FontName = "Arial"
Button2.FontSize = 12
Button2.Enabled = True
Button2.Visible = True

AddHandler(Button2.Pressed, AddressOf(Button2_Pressed))

Sub Button2_Pressed()
    NewView2.Hide()
End Sub


While True
	DoEvents(1)
Wend