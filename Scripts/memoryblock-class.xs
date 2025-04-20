// Create a new MemoryBlock instance
Var memBlock As New MemoryBlock

// Resize the MemoryBlock to 16 bytes
memBlock.Resize(16)

// Write binary data to the memory block
memBlock.WriteByte(0, 255)
memBlock.WriteShort(1, 32000)
memBlock.WriteLong(4, 123456789)
memBlock.WriteDouble(8, 3.14159)

// Read values from the memory block
Var byteVal As Integer = memBlock.ReadByte(0)
Var shortVal As Integer = memBlock.ReadShort(1)
Var longVal As Integer = memBlock.ReadLong(4)
Var doubleVal As Double = memBlock.ReadDouble(8)

Print("Byte: " + Str(byteVal))
Print("Short: " + Str(shortVal))
Print("Long: " + Str(longVal))
Print("Double: " + Str(doubleVal))

// Resize the MemoryBlock to 32 bytes
memBlock.Resize(32)
Print("Resized MemoryBlock to 32 bytes")

// Create a second MemoryBlock instance for copying data
Var copyBlock As New MemoryBlock
copyBlock.Resize(32)
// Use the ToString method to pass the source instance as a string.
// The ToString will return a string like "plugin:<handle>:tostring".
copyBlock.CopyData(0, 1, 0, 16)
Print("Copied 16 bytes to new MemoryBlock")

// Destroy both memory blocks
memBlock.Close()
copyBlock.Close()
Print("MemoryBlocks destroyed.")
