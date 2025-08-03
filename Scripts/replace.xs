// -----------------------------------------------------------------------------
// Demo: String Replacement Functions in CrossBasic
// This example demonstrates how to use `Replace` and `ReplaceAll` functions
// to perform substring replacements in strings. `Replace` affects the first match
// only (or specific instances), while `ReplaceAll` affects all occurrences.
// -----------------------------------------------------------------------------

// Create a string with a phrase to be partially removed
Var s As String = "hello world"

// Use Replace to remove " world" from the string
// Expected output: "hello"
Print(Replace(s, " world", ""))

// Create another string with a longer sentence
Var fox As String = "The quick brown fox jumped over the lazy dog."

// Use ReplaceAll to replace all occurrences of "the" with "a"
// Note: ReplaceAll is case-sensitive, so "The" remains unchanged
// Expected output: "The quick brown fox jumped over a lazy dog."
Print(ReplaceAll(fox, "the", "a"))
