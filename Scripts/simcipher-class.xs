// Create a new SimCipher instance.
Var cipher As New SimCipher

// Generate new keys.
Var keys As String = cipher.GenerateKeys(1024)
Print "Raw Generated Keys: " + keys

// Split the keys string into its parts.
// Format: modulus|publicExponent|modulus|privateExponent
Var parts() As String = Split(keys, "|")

If parts.LastIndex() = 3 Then
  Var modulus    As String = parts(0)    // same as parts(2)
  Var publicExp  As String = parts(1)
  Var privateExp As String = parts(3)
  print("made")
  // Display each piece
  Print "Public Modulus (n):    " + modulus
  Print "Public Exponent (e):   " + publicExp
  Print "Private Modulus (n):   " + modulus
  Print "Private Exponent (d):  " + privateExp
Else
  Print "Unexpected key format – got " + Str(parts.LastIndex() + 1) + " parts."
End If

// (Optionally) Load keys into the instance.
// Var loadResult As String = cipher.LoadKeys(modulus, privateExp)
// Print "Load Keys Result: " + loadResult

// Encrypt / Decrypt / Sign / Verify as before...
Var plaintext As String = "Hello, secret world!"
Var ciphertext As String = cipher.EncryptMessage(plaintext)
Print "Ciphertext: " + ciphertext

Var decrypted As String = cipher.DecryptMessage(ciphertext)
Print "Decrypted Text: " + decrypted

Var signature As String = cipher.SignMessage("This is a signed message.")
Print "Signature: " + signature

Var verifyResult As String = cipher.VerifySignature("This is a signed message.", signature)
Print "Signature Verified: " + verifyResult

cipher.Close()
