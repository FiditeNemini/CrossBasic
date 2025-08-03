// -----------------------------------------------------------------------------
// Demo: SimCipher Cryptographic Operations in CrossBasic
// This example demonstrates how to use the SimCipher class for RSA-style
// cryptographic operations, including key generation, encryption/decryption,
// signing, and signature verification.
// -----------------------------------------------------------------------------

// Create a new instance of the SimCipher cryptographic class
Var cipher As New SimCipher

// Generate a 1024-bit RSA-like keypair
// The returned format is: "modulus|publicExponent|modulus|privateExponent"
Var keys As String = cipher.GenerateKeys(1024)
Print "Raw Generated Keys: " + keys

// Split the key string into parts for clarity and access
Var parts() As String = Split(keys, "|")

// Check if the format is as expected (4 parts total)
If parts.LastIndex() = 3 Then
  // Extract the key components
  Var modulus    As String = parts(0)  // Also serves as private modulus
  Var publicExp  As String = parts(1)
  Var privateExp As String = parts(3)

  Print("made")  // Optional debug confirmation

  // Display each extracted key piece
  Print "Public Modulus (n):    " + modulus
  Print "Public Exponent (e):   " + publicExp
  Print "Private Modulus (n):   " + modulus
  Print "Private Exponent (d):  " + privateExp
Else
  // Output an error message if the key format is incorrect
  Print "Unexpected key format – got " + Str(parts.LastIndex() + 1) + " parts."
End If

// Optionally load specific keys into the cipher instance (if using externally provided keys)
// Var loadResult As String = cipher.LoadKeys(modulus, privateExp)
// Print "Load Keys Result: " + loadResult

// Encrypt a plain message using the public key
Var plaintext As String = "Hello, secret world!"
Var ciphertext As String = cipher.EncryptMessage(plaintext)
Print "Ciphertext: " + ciphertext

// Decrypt the ciphertext using the private key
Var decrypted As String = cipher.DecryptMessage(ciphertext)
Print "Decrypted Text: " + decrypted

// Digitally sign a message using the private key
Var signature As String = cipher.SignMessage("This is a signed message.")
Print "Signature: " + signature

// Verify the signature using the public key
Var verifyResult As String = cipher.VerifySignature("This is a signed message.", signature)
Print "Signature Verified: " + verifyResult

// Clean up by destroying the cipher instance
cipher.Close()
