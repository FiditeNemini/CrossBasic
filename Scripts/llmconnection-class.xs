// Create a new LLMConnection instance
Var llm As New LLMConnection

// Set API host (for example, using a local Ollama server URL)
llm.APIHost = "http://127.0.0.1:11434/v1/"

// Set API key and organization if needed (for OpenAI, etc.)
// For Ollama, these may be empty.
llm.APIKey = ""
llm.Organization = ""

// Create a text completion request.
Var completionResponse As String = llm.CreateCompletion("llama3.2:latest", "Write an essay about Spartanburg, SC in markdown format", 1024, 0.7)
Print "Text Completion Response: "
PrintColorMarkdown(completionResponse)

// (Optional) Create an image generation request. **API and Model must support image generation -ie OpenAI
// Var imageUrl As String = llm.CreateImage("llama3.2:latest", "A futuristic cityscape at night", 1, "512x512")
// Print "Generated Image URL: " + imageUrl

// Create another text completion request.
Var outlineResponse As String = llm.CreateCompletion("llama3.2:latest", "Create an outline for a fictional novel for young adults ages 18-22.", 1024, 0.7)
Print("Novel Outline: ")
PrintColorMarkdown(outlineResponse)

// Destroy the LLMConnection instance when done.
llm.Close()
