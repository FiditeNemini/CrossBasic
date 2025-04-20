Dim host As String = "http://127.0.0.1:11434/v1/" ' Ollama local server example
Dim apiKey As String = "" ' Only required for OpenAI and Commercial API
Dim organization As String = "" ' Only required for OpenAI and Commercial API
Dim model As String = "llama3.2:latest" ' Change to the model you want to use
Dim prompt As String
Dim maxTokens As Integer = 256
Dim temperature As Double = 0.7
Dim userInput As String

' Create a new LLMConnection instance.
Dim llm As New LLMConnection

' Set API host, API key, and organization.
llm.APIHost = host
llm.APIKey = apiKey
llm.Organization = organization

' Define the system prompt.
Dim systemPrompt As String = "Role: You are a friendly pirate assistant named Blacksail. Argh Matey!!! " + _
        "You speak like a pirate, act like a pirate, and never break character. You only know the pirate life, " + _
        "anything else is out of your realm of knowledge." + EndOfLine

PrintColorMarkdown("## Welcome to Ollama Chat - Pirate Style! Get ready to speak with a pirate. Enter 'exit()' at any time to quit." + EndOfLine)

prompt = systemPrompt

While True
    PrintColorMarkdown("### User:")
    userInput = Input()
    If userInput = "exit()" Then 
        Return 0
    End If
    prompt = prompt + "user: " + userInput
    ' Create a text completion request using the LLMConnection instance.
    Dim response As String = llm.CreateCompletion(model, prompt, maxTokens, temperature)
    PrintColorMarkdown("** Blacksail:** " + response)
    prompt = prompt + "assistant: " + response + EndOfLine
Wend

llm.close()
