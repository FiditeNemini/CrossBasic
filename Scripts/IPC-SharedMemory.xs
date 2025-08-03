' XSharedIPC Plugin Demo – exercises every feature of the XSharedIPC Shared Memory class

Dim ipc As New XSharedIPC

' ──────────────────────────────────────────────────────────────────────────────
' 1) Shared Memory: ints, strings & arrays
' ──────────────────────────────────────────────────────────────────────────────

ipc.CreateSharedMemory("DemoMem", 1024)

ipc.SetInt("MyInt", 12345)
Print "Shared MyInt -> " + Str(ipc.GetInt("MyInt"))

ipc.SetString("MyStr", "Hello from CrossBasic!")
Print "Shared MyStr -> " + ipc.GetString("MyStr")

ipc.CreateIntArray("MyArr", 5)
For i = 0 To ipc.GetArraySize("MyArr") - 1
  ipc.SetArrayInt("MyArr", i, i * 10)
Next i

Print "Shared MyArr contents:"
For i = 0 To ipc.GetArraySize("MyArr") - 1
  Print "  [" + Str(i) + "] = " + Str(ipc.GetArrayInt("MyArr", i))
Next i


' ──────────────────────────────────────────────────────────────────────────────
' 2) Named Mutex: synchronization across processes
' ──────────────────────────────────────────────────────────────────────────────

ipc.CreateNamedMutex("DemoMtx")

Print "Locking mutex..."
ipc.LockMutex()
Print "Inside critical section"
ipc.UnlockMutex()
Print "Mutex unlocked."

ipc.RemoveNamedMutex("DemoMtx")


' ──────────────────────────────────────────────────────────────────────────────
' 3) Clean up
' ──────────────────────────────────────────────────────────────────────────────

ipc.Close()
