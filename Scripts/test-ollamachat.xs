// -----------------------------------------------------------------------------
// Demo: Pirate Roleplay Chat with LLM using Ollama in CrossBasic
// This interactive chatbot connects to a local Ollama server using the LLMConnection
// plugin. It plays the role of "Blacksail", a pirate character, and stays in-character
// throughout the session. Enter 'exit()' to end the conversation.
// -----------------------------------------------------------------------------

// Configuration for the LLM
Dim host As String = "http://127.0.0.1:11434/v1/"        // Ollama local server URL
Dim apiKey As String = ""                                // Optional for commercial APIs
Dim organization As String = ""                          // Optional for commercial APIs
Dim model As String = "llama3.2:latest"                  // Model to be used for completions

// Other variables
Dim prompt As String
Dim maxTokens As Integer = 256
Dim temperature As Double = 0.7
Dim userInput As String

// Create a new LLMConnection instance
Dim llm As New LLMConnection

// Assign server and credentials (Ollama doesn't require API key/org)
llm.APIHost = host
llm.APIKey = apiKey
llm.Organization = organization

// Define the system-level prompt to guide the model's behavior
Dim systemPrompt As String = "Role: You are a friendly pirate assistant named Blacksail. Argh Matey!!! " + _
        "You speak like a pirate, act like a pirate, and never break character. You only know the pirate life, " + _
        "anything else is out of your realm of knowledge." + EndOfLine

// Initial greeting
PrintColorMarkdown("## Welcome to Ollama Chat - Pirate Style! Get ready to speak with a pirate. Enter 'exit()' at any time to quit." + EndOfLine)

// Initialize the conversation prompt with the system instructions
prompt = systemPrompt

// Start interactive chat loop
While True
    PrintColorMarkdown("### User:")
    
    // Read user input
    userInput = Input()
    
    // Exit condition
    If userInput = "exit()" Then 
        Return 0
    End If

    // Append user input to the ongoing prompt history
    prompt = prompt + "user: " + userInput + EndOfLine

    // Send the prompt to the model and get the response
    Dim response As String = llm.CreateCompletion(model, prompt, maxTokens, temperature)

    // Display response in pirate voice
    PrintColorMarkdown("**Blacksail:** " + response)

    // Append assistant response to prompt for continuity
    prompt = prompt + "assistant: " + response + EndOfLine
Wend

// Close the connection (never reached in current loop, but good practice)
llm.Close()
