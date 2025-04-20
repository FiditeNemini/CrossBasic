#tag Module
Protected Module CrossBasic
	#tag Method, Flags = &h0
		Sub GetFunctions()
		  var input as string = "print " + chr(34) + chr(34)
		  
		  Dim output As String = RunCrossBasic(input.ReplaceLineEndings(EndOfLine), true).DefineEncoding(Encodings.UTF8).ReplaceLineEndings(EndOfLine)
		  
		  var lines() as String = output.Split(EndOfLine)
		  
		  Functions.RemoveAll()
		  classes.RemoveAll()
		  
		  var ld as string = "[DEBUG] Loaded plugin function: "
		  var cd as string = "[DEBUG] Loaded plugin class: "
		  var wa as string
		  
		  for i as integer = 0 to lines.LastIndex
		    if lines(i).IndexOf(ld) > -1 then
		      
		      wa = " with arity "
		      
		      var f as String = lines(i).NthField(ld,2).NthField(" ",1).Trim
		      var a as string = lines(i).NthField(wa, 2).NthField(" ", 1).Trim
		      var d as String = lines(i).NthField(ld + f + wa + a + " from ", 2).Trim
		      
		      var n as new PluginFunction
		      n.Name = f
		      n.Arity = a.ToInteger
		      n.Location = d
		      
		      functions.Add (n)
		    end if
		    
		    if lines(i).IndexOf(cd) > -1 then
		      
		      wa = " from "
		      
		      var f as string = lines(i).NthField(cd,2).NthField(wa,1).Trim
		      var d as string = lines(i).NthField(cd + f + wa,2).Trim
		      
		      var n as new PluginClass
		      n.Name = f
		      n.Location = d
		      
		      classes.Add (n)
		    end if
		    
		    
		  next
		  
		End Sub
	#tag EndMethod

	#tag Method, Flags = &h0
		Function RunCrossBasic(code as String, Optional enableDebug as Boolean = False) As String
		  // Declare the external function from the DLL.
		  #If TargetWindows Then
		    Soft Declare Function CompileAndRun Lib "crossbasic.dll" (code As CString, debugEnabled as Boolean) As CString
		  #ElseIf TargetMacOS Then
		    Soft Declare Function CompileAndRun Lib "crossbasic.dylib" (code As CString, debugEnabled as Boolean) As CString
		  #Else
		    Soft Declare Function CompileAndRun Lib "crossbasic.so" (code As CString, debugEnabled as Boolean) As CString
		  #EndIf
		  
		  // Call the external function.
		  
		  var c as CString = code
		  
		  Dim cResult As String = cstr(CompileAndRun(code, enableDebug))
		  
		  // Convert the C-string to a Xojo String and return it.
		  Return cResult
		  
		End Function
	#tag EndMethod


	#tag Property, Flags = &h0
		classes() As PluginClass
	#tag EndProperty

	#tag Property, Flags = &h0
		functions() As PluginFunction
	#tag EndProperty


	#tag ViewBehavior
		#tag ViewProperty
			Name="Name"
			Visible=true
			Group="ID"
			InitialValue=""
			Type="String"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Index"
			Visible=true
			Group="ID"
			InitialValue="-2147483648"
			Type="Integer"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Super"
			Visible=true
			Group="ID"
			InitialValue=""
			Type="String"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Left"
			Visible=true
			Group="Position"
			InitialValue="0"
			Type="Integer"
			EditorType=""
		#tag EndViewProperty
		#tag ViewProperty
			Name="Top"
			Visible=true
			Group="Position"
			InitialValue="0"
			Type="Integer"
			EditorType=""
		#tag EndViewProperty
	#tag EndViewBehavior
End Module
#tag EndModule
