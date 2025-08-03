// -----------------------------------------------------------------------------
// Demo: Preemptive Threading in CrossBasic
// This script demonstrates how to create and run a preemptive thread using the
// `XThread` class. Unlike cooperative threads, preemptive threads can interrupt
// other threads and run independently without yielding manually.
// -----------------------------------------------------------------------------

Print("Spinning-up 10 separate preemptive worker threads")

// Define the event handler for the thread's Run event.
// This subroutine simulates CPU work for 10 iterations.
Sub Thread2Run()
  For i As Integer = 1 To 10
    // Output current loop iteration and thread state
    Print("Preemptive iteration " + Str(i) + " (Tag=" + thr2.Tag + ") ThreadState: " + Str(thr2.ThreadState))

    // Simulate CPU-bound task
    Dim sum As Integer = 0
    For j As Integer = 1 To 500
      sum = sum + j
    Next
  Next

  // Notify that the thread has finished
  Print("Preemptive thread finished.")
End Sub

// =================================================================================
// Main Program Execution
// =================================================================================

// Create a new instance of XThread for preemptive execution
Dim thr2 As New XThread

// Set a descriptive tag (useful for logging/debugging)
thr2.Tag = "PreemptDemo"

// Assign the event handler for OnRun
AddHandler(thr2.OnRun, AddressOf(Thread2Run))

// Set the thread type to 1 (preemptive)
thr2.Type = 1

// Start the thread
thr2.Start()

// Since this is a preemptive thread, there's no need to Sleep
// It runs on a separate thread pool and can interrupt other threads

// Keep the application running until the thread exits
While thr2.ThreadState <> 4  // 4 = NotRunning
  DoEvents()
Wend

// Final message after thread completes
Print("All done. Press Enter to exit.")
Sleep(1000)
