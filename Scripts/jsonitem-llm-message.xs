// -----------------------------------------------------------------------------
// Demo: JSONItem Chat Message Construction for LLMs in CrossBasic
// This example simulates a chat conversation history using the JSONItem class.
// It creates role-based messages (system, user, assistant), organizes them into
// a structured array, and appends messages dynamically. Useful for LLM/Chat API calls.
// -----------------------------------------------------------------------------

// Create a root JSON object for the chat conversation
Var conversation As New JSONItem
conversation.SetValue("Title", "Chat Conversation")

// Create a JSON array to hold chat message entries
Var messages As New JSONItem
messages.Load("[]")  // Initialize as empty array

// Create the system message and set its role and content
Var systemMsg As New JSONItem
systemMsg.SetValue("role", "system")
systemMsg.SetValue("content", "You are a helpful assistant.")

// Create the first user message
Var userMsg As New JSONItem
userMsg.SetValue("role", "user")
userMsg.SetValue("content", "Hello, assistant!")

// Create the assistant's reply message
Var assistantMsg As New JSONItem
assistantMsg.SetValue("role", "assistant")
assistantMsg.SetValue("content", "Hello! How can I help you today?")

// Add each message to the messages array as serialized JSON strings
messages.Add(systemMsg.ToString)
messages.Add(userMsg.ToString)
messages.Add(assistantMsg.ToString)

// Store the messages array inside the main conversation object using the .Handle API
conversation.SetChild("messages", messages.Handle)

// Format the output JSON with indentation for readability
conversation.Compact = False
conversation.IndentSpacing = 2

// Output the full formatted conversation JSON
Print(EndOfLine + "Conversation JSON:")
Print(conversation.ToString)

// Access the messages array again from the conversation object for iteration
Var messagesArray As JSONItem = conversation.Child("messages")
Print(EndOfLine + "Messages loaded. Count: " + Str(messagesArray.Count))

// Loop through each message in the array and print its role and content
For i As Integer = 0 To messagesArray.Count - 1
  Var msgItem As New JSONItem
  msgItem.Load(messagesArray.ValueAt(i))  // Load JSON string into object
  Print("Message " + Str(i) + " (" + msgItem.Value("role") + "): " + _
        msgItem.Value("content"))
  msgItem.Close
Next

// Create and append a new user message asking for a joke
Var newUserMsg As New JSONItem
newUserMsg.SetValue("role", "user")
newUserMsg.SetValue("content", "Can you tell me a joke?")
messages.Add(newUserMsg.ToString)

// Update the conversation object with the modified messages array
conversation.SetChild("messages", messages.Handle)

// Output the updated conversation after appending a new message
Print(EndOfLine + "After adding a new message:")
Print(conversation.ToString)

// Clean up all JSONItem instances to free memory/resources
conversation.Close
messages.Close
messagesArray.Close
systemMsg.Close
userMsg.Close
assistantMsg.Close
newUserMsg.Close
