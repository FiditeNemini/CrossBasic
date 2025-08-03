'--------------------------------------------------------------------------------
'  XCanvas + XGraphics + Animated Scene Demo
'--------------------------------------------------------------------------------

' 1) Create our main window
Dim win As New XWindow
win.Title               = "XCanvas / XGraphics - Animated Scene"
win.Width               = 650
win.Height              = 500
win.Resize              = False
win.HasMaximizeButton   = False
win.HasMinimizeButton   = False

AddHandler(win.Closing,    AddressOf(TerminateApplication))

' 6) Quit handler
Sub TerminateApplication()
  Quit()
End Sub

' 2) Create and position an XCanvas
Dim canvas As New XCanvas
canvas.Parent           = win.Handle
canvas.Graphics.Antialias = True
canvas.Left             = 20
canvas.Top              = 20
canvas.Width            = 600
canvas.Height           = 400

' 3) Scene state variables
Var rectlocx    As Integer = 50    ' bouncing oval X
Var rectlocy    As Integer = 50    ' bouncing oval Y
Var rectvelx    As Integer = 3     ' bouncing oval X velocity
Var rectvely    As Integer = 3     ' bouncing oval Y velocity

Var sunX        As Integer = 0     ' sun X
Var sunVelX     As Integer = 1     ' sun horizontal speed

Var cloudX      As Integer = canvas.Width  ' cloud starts off right edge
Var cloudVelX   As Integer = -2           ' cloud drifts left

Var houseX      As Integer = 250   ' house position
Var houseY      As Integer = 220

' 4) Paint handler
Sub CanvasPaint(g As XGraphics) As Boolean
  g.Antialias = True

  ' — Sky background
  g.DrawingColor = &c87CEEB      ' light-blue
  g.FillRect(0, 0, g.Width, g.Height)

  ' — Sun
  g.DrawingColor = &cFFFF00      ' yellow
  g.FillOval(sunX, 30, 60, 60)

  ' — Cloud (two overlapping ellipses)
  g.DrawingColor = &cFFFFFF      ' white
  g.FillOval(cloudX,    50, 80, 40)
  g.FillOval(cloudX+40, 40, 80, 50)

  ' — House walls
  g.DrawingColor = &c8B4513      ' brown
  g.FillRect(houseX, houseY, 200, 150)

  ' — Roof (filled triangle)
'   g.DrawingColor = &cFF0000      ' red
'   Dim pts() As Double
'   pts.add(houseX)
'   pts.add(houseY)
'   pts.add(houseX + 100)
'   pts.add(houseY - 100)
'   pts.add(houseX + 200)
'   pts.add(houseY)
'   g.FillPolygon(pts,3)

  ' — Bouncing ball (oval)
  g.PenSize      = 3
  g.DrawingColor = &c00FF00      ' green
  g.FillOval(rectlocx, rectlocy, 100, 100)

  ' — Bounce logic
  If rectlocx <= 0 Or rectlocx + 100 >= g.Width Then rectvelx = -rectvelx
  If rectlocy <= 0 Or rectlocy + 100 >= g.Height Then rectvely = -rectvely
  rectlocx = rectlocx + rectvelx
  rectlocy = rectlocy + rectvely

  Return True
End Sub

' 5) Timer to drive animation
Sub TimerFired()
  ' Move sun; wrap when off right edge
  sunX = sunX + sunVelX
  If sunX > canvas.Width Then sunX = -60

  ' Move cloud; wrap when off left edge
  cloudX = cloudX + cloudVelX
  If cloudX < -200 Then cloudX = canvas.Width

  canvas.Invalidate()  ' trigger repaint
End Sub



win.Show()
canvas.Invalidate()

var t as New XTimer
t.Period = 16
t.RunMode = 2
' 7) Wire up events and start
AddHandler(canvas.Paint,   AddressOf(CanvasPaint))

AddHandler( t.Action, AddressOf(TimerFired))

t.Enabled = True



' 8) Keep the app alive
While True
  DoEvents(1)
Wend
