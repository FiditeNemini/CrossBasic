Var v As Variant
v = 42
Print IIF(v = 42, "forty-two", "no match")  // prints "forty-two"

v = "hello"
Print IIF(v = "hello", "greeting", "??")   // prints "greeting"

Var c As Color = &cFF0000
Print IIF(c = &cFF0000, "red", "not red")  // prints "red"

Var t As String = "test1"
Print IIF(t = "test1", "yes", "no")  // prints “yes”

Print IIF(5 > 3, "yes", "no")   // prints "yes"
