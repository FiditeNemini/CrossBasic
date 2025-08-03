// -----------------------------------------------------------------------------
// Demo: MemoryBlock Usage in CrossBasic
// This example demonstrates how to allocate, write to, read from, resize, and
// copy data between memory blocks using the MemoryBlock class. It showcases
// binary manipulation, in-memory operations, and safe memory cleanup.
// -----------------------------------------------------------------------------

// Create a new MemoryBlock instance for binary data manipulation
Var memBlock As New MemoryBlock

// Resize the memory block to 16 bytes
memBlock.Resize(16)

// Write data to specific byte offsets:
// Write a single byte (value 255) at offset 0
memBlock.WriteByte(0, 255)

// Write a 2-byte short integer (value 32000) at offset 1
memBlock.WriteShort(1, 32000)

// Write a 4-byte long integer (value 123456789) at offset 4
memBlock.WriteLong(4, 123456789)

// Write an 8-byte double-precision float (value 3.14159) at offset 8
memBlock.WriteDouble(8, 3.14159)

// Read the values back from the same memory block offsets
Var byteVal   As Integer = memBlock.ReadByte(0)
Var shortVal  As Integer = memBlock.ReadShort(1)
Var longVal   As Integer = memBlock.ReadLong(4)
Var doubleVal As Double  = memBlock.ReadDouble(8)

// Print out the retrieved values
Print("Byte: "   + Str(byteVal))
Print("Short: "  + Str(shortVal))
Print("Long: "   + Str(longVal))
Print("Double: " + Str(doubleVal))

// Resize the memory block to 32 bytes (in-place expansion)
memBlock.Resize(32)
Print("Resized MemoryBlock to 32 bytes")

// Create a second MemoryBlock for copying data into
Var copyBlock As New MemoryBlock
copyBlock.Resize(32)

// Copy 16 bytes from memBlock to copyBlock, starting from offset 0 in both
copyBlock.CopyData(0, memBlock, 0, 16)
Print("Copied 16 bytes to new MemoryBlock")

// Clean up memory by explicitly closing the memory blocks
memBlock.Close()
copyBlock.Close()
Print("MemoryBlocks destroyed.")
