// -----------------------------------------------------------------------------
// Demo: Cooperative Threading in CrossBasic
// This script demonstrates how to create and run a cooperative thread using the
// XThread class. The thread yields control by calling Sleep inside its execution,
// enabling smooth cooperative multitasking with the main thread.
// -----------------------------------------------------------------------------

Print("Spinning-up 10 cooperative worker threads")

// Define the event handler for the thread's Run event.
// This subroutine will be executed when the thread starts running.
Sub Thread1Run()
  For i As Integer = 1 To 10
    // Output current loop iteration and thread state
    Print("Cooperative iteration " + Str(i) + " (Tag=" + thr1.Tag + ") ThreadState: " + Str(thr1.ThreadState))
    
    // Yield cooperatively by sleeping (non-blocking for main thread)
    thr1.Sleep(50, False)  // Sleep 50ms without blocking others
  Next

  // After finishing, notify and stop the thread
  Print("Cooperative thread finished.")
  thr1.Stop()
End Sub

// =================================================================================
// Main Program Execution
// =================================================================================

// Create a new cooperative thread instance
Dim thr1 As New XThread

// Tag the thread with an identifier (useful for debugging/logging)
thr1.Tag = "CoopDemo"

// Attach the OnRun event to the Thread1Run handler
AddHandler(thr1.OnRun, AddressOf(Thread1Run))

// Set the thread type to cooperative (0 = cooperative, 1 = preemptive)
thr1.Type = 0

// Start the thread
thr1.Start()

// Let the thread begin execution before proceeding
Sleep(50)

// Monitor the thread’s execution state
While thr1.ThreadState <> 4  // 4 = NotRunning
  DoEvents()  // Allow thread execution and GUI/event loop processing
Wend

// Final cleanup
Print("All done. Press Enter to exit.")
Sleep(1000)
