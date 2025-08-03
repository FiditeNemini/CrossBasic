'' =============================================================================
'' A full demonstration of all XScreen plugin features.
'' Author: Matthew A. Combatti – Simulanics / Xojo Developers Studio
''
'' This script displays full screen and monitor info for all connected displays.
'' It includes display count, main screen properties, and a loop to list all
'' screen attributes like resolution, color depth, scale, and work area bounds.
'' =============================================================================

' Create a root screen instance to interact with display APIs
Dim root As New XScreen

' ------------------------------------------------------------------------
' 1) Print global screen metrics: number of displays and last display index
' ------------------------------------------------------------------------
Print("=== XScreen Global Info ===")
Print("DisplayCount:     " + Str(root.DisplayCount))
Print("LastDisplayIndex: " + Str(root.LastDisplayIndex))
Print("")

' ------------------------------------------------------------------------
' 2) Show popup with details about the primary display (index 0)
' ------------------------------------------------------------------------
Dim main As XScreen = root.DisplayAt(0)
MessageBox("Main Display:" + EndOfLine + _
           "  Name:        " + main.Name + EndOfLine + _
           "  Resolution:  " + Str(main.Width) + " x " + Str(main.Height) + EndOfLine + _
           "  ScaleFactor: " + Str(main.ScaleFactor) + EndOfLine + _
           "  ColorDepth:  " + Str(main.ColorDepth))

Print("")  ' Separate output visually in console

' ------------------------------------------------------------------------
' 3) Loop through all connected displays and print their properties
' ------------------------------------------------------------------------
For i As Integer = 0 To root.LastDisplayIndex
  Dim scr As XScreen = root.DisplayAt(i)
  Print("=== Display Index " + Str(i) + " ===")
  Print(" Name:                 " + scr.Name)
  Print(" Monitor Device:       " + scr.ScreenDisplayMonitor)
  Print(" Friendly Description: " + scr.Description)
  Print(" Fullscreen Width:     " + Str(scr.Width))
  Print(" Fullscreen Height:    " + Str(scr.Height))
  Print(" ScaleFactor:          " + Str(scr.ScaleFactor))
  Print(" ColorDepth:           " + Str(scr.ColorDepth))
  Print(" WorkArea Left:        " + Str(scr.AvailableLeft))
  Print(" WorkArea Top:         " + Str(scr.AvailableTop))
  Print(" WorkArea Width:       " + Str(scr.AvailableWidth))
  Print(" WorkArea Height:      " + Str(scr.AvailableHeight))
  Print("")  ' Add spacing between displays
Next

' Final console message
Print("Demo complete.")
