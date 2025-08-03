// -----------------------------------------------------------------------------
// Demo: String Manipulation Functions in CrossBasic
// This example demonstrates trimming, case transformations, and substring
// operations including `Left`, `Right`, `Middle`, `Replace`, and `ReplaceAll`.
// -----------------------------------------------------------------------------

// Define a string with leading and trailing spaces
Dim s As String = "   hello world!   "

// Trim leading and trailing whitespace
Dim v As String = Trim(s)
Print("Length of '" + v + "' = " + Str(Len(v)))  // Expected: 12
Print(v)                                         // Expected: "hello world!"

// Print the rightmost 6 characters from the trimmed string
Print(Right(v, 6))                               // Expected: "world!"

// Print the leftmost 8 characters from the original (untrimmed) string
Print(Left(s, 8))                                // Expected: "   hello"

// Convert the original string to title case (capitalizes first letters)
Print(TitleCase(s))                              // Expected: "   Hello World!   "

// Convert the original string to all lowercase
Print(LowerCase(s))                              // Expected: "   hello world!   "

// Convert the original string to all uppercase
Print(UpperCase(s))                              // Expected: "   HELLO WORLD!   "


// Demonstrate substring and replacement functions
Dim s As String = "Hello, Open Source XojoScript World!"

// Extract a substring starting at position 8 (1-based), length 11
Print(Middle(s, 8, 11))                          // Expected: "Open Source"

// Replace first occurrence of "Open Source" with "Open-Source"
Print(Replace(s, "Open Source", "Open-Source"))  // Expected: "Hello, Open-Source XojoScript World!"

// Replace all occurrences of lowercase "o" with "0" (case-sensitive)
Print(ReplaceAll(s, "o", "0"))                   // Expected: "Hell0, Open S0urce X0j0Script W0rld!"
