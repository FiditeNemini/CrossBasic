**Windows-Only Tool**

Sandbox is a tool-extension for CrossBasic to run untrusted or unsigned external processes in their own sandbox. Sandboxed applications may only access the specified paths provided on the commandline. Memory is also isolated and containerized for the running process, isolating it from the host operating system.

Usage:
sandbox appname.exe/bat --dirs <../../dir1, /mypath/dir2, ...>

*Note* 
Sandbox has not been yet integrated into CrossBasic, but is fully implemented. CrossBasic will utilize this tool-extension to run unsigned or untrusted code and processes safely.