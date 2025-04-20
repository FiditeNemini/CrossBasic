' Define the callback function that will be invoked when the trigger event fires.
Sub TimerTrigger(currentTime As String)
  Print("Timer triggered at: " + currentTime)
End Sub

' Create a new instance of MyInstance.
Dim inst As New MyInstance()

' Set class instance value property.
inst.Value = 42.2

' Register the event handler.
' The getter for inst.OnTrigger returns a string like "MyInstance:1:OnTrigger"
' so that AddHandler accepts it. <pluginName>:<pluginInstanceId>:<eventName>
AddHandler(inst.OnTrigger, AddressOf(TimerTrigger))

' Use some instance methods.
Print("Instance Value: " + Str(inst.Value))
Dim mval as Double = inst.MultiplyTwoNumbers(2, 42)
Print("MultiplyTwoNumbers result: " + Str(mval))

Print("Instance Max Value Contant:" + str(inst.MaxValue))

While true
  Doevents()
  sleep(1000)
wend
