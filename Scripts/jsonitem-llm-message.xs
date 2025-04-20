// --- OpenAI LLM Message JSON Array Demo ---

// Create a JSONItem for the conversation (object).
Var conversation As New JSONItem
conversation.SetValue("Title", "Chat Conversation")

// Create a JSONItem for the messages array.
Var messages As New JSONItem
messages.RemoveAll()  ' Ensure it's an empty array.
messages.Load("[]")   ' Force it to become an array if needed.

// Create a system message.
Var systemMsg As New JSONItem
systemMsg.SetValue("role", "system")
systemMsg.SetValue("content", "You are a helpful assistant.")

// Create a user message.
Var userMsg As New JSONItem
userMsg.SetValue("role", "user")
userMsg.SetValue("content", "Hello, assistant!")

// Create an assistant message.
Var assistantMsg As New JSONItem
assistantMsg.SetValue("role", "assistant")
assistantMsg.SetValue("content", "Hello! How can I help you today?")

// Instead of storing messages as strings, add them as child JSONItems.
messages.Add(systemMsg.ToString) 
messages.Add(userMsg.ToString)
messages.Add(assistantMsg.ToString)

// For this demo, we simply set the messages property using the new method:
conversation.SetChild("messages", messages.Handle)

// Optionally, set formatting properties.
conversation.Compact = False
conversation.IndentSpacing = 2

// Output the full conversation JSON.
Var convJson As String = conversation.ToString
Print EndofLine + "Conversation JSON:"
Print convJson

// --- Loop Through the Messages Array ---
// Extract the "messages" property into a new JSONItem.
Var messagesArray As New JSONItem
messagesArray.Load(conversation.Value("messages"))

Print EndofLine + "Messages Loaded. Count# : " + MessagesArray.Count().ToString

Print EndofLine + "Looping through messages:"
For i As Integer = 0 To messagesArray.Count() - 1
    Var msgStr As String = messagesArray.ValueAt(i)
    print("MSGSTR = : " + msgstr)
    Var msgItem As New JSONItem
    msgItem.Load(msgStr)
    Var role As String = msgItem.Value("role")
    Var content As String = msgItem.Value("content")
    Print "Message " + Str(i) + " (" + role + "): " + content
    msgItem.Close()
Next

// Append a new user message.
Var newUserMsg As New JSONItem
newUserMsg.SetValue("role", "user")
newUserMsg.SetValue("content", "Can you tell me a joke?")
messages.Add(newUserMsg.ToString)

// Update the conversation.
conversation.SetChild("messages", messages.Handle)
Print EndofLine + "After adding a new message, conversation JSON:"
Print conversation.ToString

// Cleanup: destroy all JSONItem instances.
conversation.Close()
messages.Close()
messagesArray.Close()
systemMsg.Close()
userMsg.Close()
assistantMsg.Close()
newUserMsg.Close()
