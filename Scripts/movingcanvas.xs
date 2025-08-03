'--------------------------------------------------------------------------------
'  XCanvas + XGraphics + XPicture Demo with Bouncing Oval
'--------------------------------------------------------------------------------

' Create our main window
Dim win As New XWindow
win.Title = "XCanvas / XGraphics - Bouncing Ball Demo"
win.Width  = 650
win.Height = 500
win.resize = False
win.HasMaximizeButton = False
win.HasMinimizeButton = False

'------------------------------------------------------------------------
'  2) Create and position an XCanvas
'------------------------------------------------------------------------
Dim canvas As New XCanvas
canvas.Parent    = win.Handle
canvas.Graphics.Antialias = True
canvas.Left      = 20
canvas.Top       = 20
canvas.Width     = 600
canvas.Height    = 400

'------------------------------------------------------------------------
'  3) Oval position & velocity
'------------------------------------------------------------------------
Var rectlocx As Integer = 50
Var rectlocy As Integer = 50
Var rectvelx As Integer = 3    ' horizontal speed
Var rectvely As Integer = 3    ' vertical speed

Sub CanvasPaint(g As XGraphics) As Boolean
  ' turn on antialiasing
  g.Antialias = True

  ' clear background white
  g.DrawingColor = &c000000
  g.FillRect(0, 0, g.Width, g.Height)

  ' draw the oval
  g.PenSize      = 3
  g.DrawingColor = &c00FF00
  g.FillOval(rectlocx, rectlocy, 100, 100)

  ' bounce off left/right edges
  If rectlocx <= 0 Or rectlocx + 100 >= g.Width Then
    rectvelx = -rectvelx
  End If

  ' bounce off top/bottom edges
  If rectlocy <= 0 Or rectlocy + 100 >= g.Height Then
    rectvely = -rectvely
  End If

  ' update position
  rectlocx = rectlocx + rectvelx
  rectlocy = rectlocy + rectvely

  Return True
End Sub

Sub TerminateApplication()
  Quit()
End Sub

' hook up events
AddHandler(canvas.Paint, AddressOf(CanvasPaint))
AddHandler(win.Closing, AddressOf(TerminateApplication))

' Show the window and start the event loop
win.Show()
canvas.Invalidate()

'------------------------------------------------------------------------
'  Timer to continuously repaint
'------------------------------------------------------------------------
Dim tmr As New XTimer
tmr.Period  = 1    ' ms
tmr.RunMode = 2    ' multiple firing
AddHandler(tmr.Action, AddressOf(TimerFired))
tmr.Enabled = True

Sub TimerFired()
  canvas.Invalidate()
End Sub

' keep the app alive
While True
  DoEvents(1)
Wend
