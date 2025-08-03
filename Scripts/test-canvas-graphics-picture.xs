'--------------------------------------------------------------------------------
'  XCanvas + XGraphics + XPicture Demo
'--------------------------------------------------------------------------------

' Create our main window
Dim win As New XWindow
win.Title = "XCanvas / XGraphics / XPicture Demo"
win.Width  = 650
win.Height = 500

'------------------------------------------------------------------------
'  1) Load an image from disk into an XPicture
'------------------------------------------------------------------------
Print("Loaded XPicture Dimensions")
Dim pic As New XPicture
pic.Load("ico.png")    ' ← replace with a valid path
'------------------------------------------------------------------------
'  4) Save a snapshot of our picture object back out to disk
'------------------------------------------------------------------------
print(str(pic.width) + ", " + str(pic.height))
pic.Save("canvas_snapshot.png")
'------------------------------------------------------------------------
'  2) Create and position an XCanvas
'------------------------------------------------------------------------
Dim canvas As New XCanvas
canvas.Parent = win.Handle
canvas.Graphics.Antialias = True
canvas.Left   = 20
canvas.Top    = 20
canvas.Width  = 600
canvas.Height = 400


'------------------------------------------------------------------------
'  3) Handle the Paint event to draw shapes & the picture
'------------------------------------------------------------------------

Sub CanvasPaint(g as XGraphics) as Boolean
  g.Antialias = True
  ' Draw a blue diagonal line
  Print("PAINTING")
  g.clear()
  g.PenSize = 3
  g.DrawingColor = &c0000FF
  g.DrawLine(0, 0, canvas.Width, canvas.Height)
  
  ' Draw a red rectangle outline
  g.DrawingColor = &cFF0000
  g.DrawRect(50, 50, 150, 100)
  
  ' Fill a green oval
  g.DrawingColor = &c00FF00
  g.FillOval(250, 50, 100, 100)
  
  ' Draw some black text
  g.DrawingColor = &c000000
  g.FontName = "Arial"
  g.FontSize = 30
  g.DrawText("Hello, XCanvas!", 20, canvas.Height - 80)
  
  ' Draw our loaded picture in the lower right
  Dim picW As Integer = 100
  Dim picH As Integer = 100
  Dim px   As Integer = canvas.Width - picW - 10
  Dim py   As Integer = canvas.Height - picH - 10

  g.DrawPicture(pic, 0, 0, canvas.Width, canvas.height)
  return true
  
End Sub

Sub MouseDown(x as Integer, y as Integer)
  Print("MouseDown")
End Sub

Sub MouseUp(x as Integer, y as Integer)
  Print("MouseUp")
End Sub

Function MouseMove(x as Integer, y as Integer)
  Print("x=" + str(x) + " y=" + str(y))
End Sub

Sub DoubleClick()
  Print("DoubleClicked")
End Sub

Sub TerminateApplication()
  Quit()
End Sub

AddHandler(canvas.Paint, AddressOf(CanvasPaint))
AddHandler(canvas.MouseDown, AddressOf(MouseDown))
AddHandler(canvas.MouseUp, AddressOf(MouseUp))
AddHandler(canvas.MouseMove, AddressOf(MouseMove))
AddHandler(canvas.DoubleClick, AddressOf(DoubleClick))
AddHandler(win.Closing, Addressof(TerminateApplication))


' Show the window and start the event loop
win.Show()
canvas.Invalidate()

Print("XPicture Canvas.Backdrop Dimensions")
var gg as XPicture = Canvas.Backdrop
print(str(gg.width) + ", " + str(gg.height))
gg.save("canvas_image.png")

Print("XGraphics Canvas.Graphics Dimensions")
var cg as XGraphics = Canvas.Graphics
print(str(Canvas.width) + ", " + str(Canvas.height))
print(str(cg.width) + ", " + str(cg.height))
cg.savetofile("canvas-graphics.png")


while True
 DoEvents(1)
Wend
