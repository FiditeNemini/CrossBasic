// -----------------------------------------------------------------------------
// Demo: LLMConnection Usage in CrossBasic
// This example demonstrates how to interact with a Large Language Model (LLM)
// using the LLMConnection class. It shows how to connect to a local LLM server
// (e.g., Ollama), send text completion requests, and format/display the responses.
// -----------------------------------------------------------------------------

// Create a new instance of the LLMConnection class for handling requests
Var llm As New LLMConnection

// Set the base API host URL for the LLM server (Ollama in this case)
llm.APIHost = "http://127.0.0.1:11434/v1/"

// Optionally set an API key and organization ID
// These fields are used by services like OpenAI but are not required for Ollama
llm.APIKey = ""
llm.Organization = ""

// Request a text completion using the model 'llama3.2:latest'
// Parameters: model name, prompt, max tokens, temperature (creativity)
Var completionResponse As String = llm.CreateCompletion( _
  "llama3.2:latest", _
  "Write an essay about Spartanburg, SC in markdown format", _
  1024, _
  0.7
)

// Output the essay response with markdown color formatting
Print "Text Completion Response: "
PrintColorMarkdown(completionResponse)

// (Optional) Uncomment this to request image generation if supported by the model/API
// Var imageUrl As String = llm.CreateImage( _
//   "gpt-4o", _
//   "A futuristic cityscape at night", _
//   1, _
//   "512x512"
// )
// Print "Generated Image URL: " + imageUrl

// Request another text completion to generate a fictional novel outline
Var outlineResponse As String = llm.CreateCompletion( _
  "llama3.2:latest", _
  "Create an outline for a fictional novel for young adults ages 18-22.", _
  1024, _
  0.7
)

// Output the outline response in markdown format
Print("Novel Outline: ")
PrintColorMarkdown(outlineResponse)

// Clean up the LLMConnection instance to release any resources
llm.Close()
