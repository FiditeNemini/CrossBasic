' CrossBasic BigInteger Math Plugin Demo

' Sample inputs
Dim num1 As String = "123456789012345678901234567890"
Dim num2 As String = "987654321098765432109876543210"
Dim precision As Integer = 50

' Perform operations
Dim sum As String
Dim difference As String
Dim product As String
Dim quotient As String
Dim remainder As String

sum = BigIntAdd(num1, num2, precision)
difference = BigIntSubtract(num1, num2, precision)
product = BigIntMultiply(num1, num2, precision)
quotient = BigIntDivide(num1, num2, precision)
remainder = BigIntModulo(num1, num2, precision)

' Display results
Print "BigIntAdd: " + sum
Print "BigIntSubtract: " + difference
Print "BigIntMultiply: " + product
Print "BigIntDivide: " + quotient
Print "BigIntModulo: " + remainder

