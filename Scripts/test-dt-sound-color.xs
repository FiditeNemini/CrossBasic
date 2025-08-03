// -----------------------------------------------------------------------------
// Demo: Ode to Joy Melody Playback in CrossBasic
// This script demonstrates generating sound using `Beep(frequency, duration)`
// while simultaneously printing colorful outputs with `PrintColor`. It plays the
// melody for "Ode to Joy" across 4 verses and randomly changes the console text color.
// -----------------------------------------------------------------------------

// Verse 1 - Frequencies (in Hz) and durations (in ms)
Var frequencies() As Integer = Array(330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 330, 294, 294)
Var durations()   As Integer = Array(400, 400, 600, 600, 600, 600, 600, 600, 600, 400, 400, 600, 600, 600, 900)

// Verse 2/4 - Slight variation to play later again
Var frequencies2() As Integer = Array(330, 330, 349, 392, 392, 349, 330, 294, 262, 262, 294, 330, 294, 262, 262)
Var durations2()   As Integer = Array(400, 400, 600, 600, 600, 600, 600, 600, 600, 400, 400, 600, 600, 600, 900)

// Verse 3 - Adds a melodic bridge
Var frequencies3() As Integer = Array(294, 294, 330, 262, 294, 330, 349, 330, 262, 294, 330, 349, 330, 294, 262, 294, 196)
Var durations3()   As Integer = Array(400, 400, 600, 600, 600, 600, 600, 600, 400, 400, 600, 600, 600, 600, 900, 600, 900)

// Subroutine: Plays a verse with given frequency and duration arrays
Sub PlayVerse(freqs As Variant, durs As Variant)
  For i As Integer = 0 To freqs.Count() - 1
    // Play each note with its corresponding duration
    Beep(freqs(i), durs(i))

    // Generate a random RGB color and convert it to HEX
    Var NR As New Random
    Var r As Integer = NR.InRange(0, 255)
    Var g As Integer = NR.InRange(0, 255)
    Var b As Integer = NR.InRange(0, 255)
    Var clr As String = RGBtoHEX(r, g, b)

    // Display time-stamped color feedback for each note
    PrintColor(clr + " ", clr)
    Print(GetCurrentDate() + " - " + GetCurrentTime())

    // Brief pause between notes
    Sleep(100)
  Next
End Sub

// Start melody playback
Print("Turn your volume up! Ode to Joy!")
PlayVerse(frequencies, durations)
PlayVerse(frequencies2, durations2)
PlayVerse(frequencies3, durations3)
PlayVerse(frequencies2, durations2)  // Repeat verse 2 for ending
