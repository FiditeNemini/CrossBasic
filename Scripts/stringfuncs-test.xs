Dim s as String = "   hello world!   "
Dim v as String = trim(s)
Print(v)         // prints "hello world!"
Print(right(v, 6))     // prints "world!"
Print(left(s, 8))      // prints "   hello"
Print(titlecase(s))    // prints "   Hello World!   "
Print(lowercase(s))    // prints "   hello world!   "
Print(uppercase(s))    // prints "   HELLO WORLD!   "

Dim s As String = "Hello, Open Source XojoScript World!"
Print(middle(s, 8, 11))  ' Should print "Open Source"
Print(Replace(s, "Open Source", "Open-Source"))
Print(ReplaceAll(s, "o", "0"))
