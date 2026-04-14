# MessagingApp

Self hosted direct messaging app written in C using sockets and the TCP protocol

## Prerequisites for compilation

CMake must be installed

# Linux and MacOS build instructions
## How to compile 
	$ git clone https://github.com/BrianO-04/MessagingApp.git/
	$ cd MessagingApp	
	$ mkdir build
	$ cmake .S . -B build
	$ cmake --build build
## How to run client
	$ ./MessagingApp {Name} {IP}
## How to run server
	Hosted on TCP port 8080
	$ ./MessagingServer

# Windows build instructions
## How to compile (PowerShell)
	$ git clone https://github.com/BrianO-04/MessagingApp.git/
	$ cd MessagingApp	
	$ mkdir build
	$ cmake .S . -B build
	$ cmake --build build --config Release
## How to run client (CMD or PowerShell)
	$ MessagingApp.exe {Name} {IP}
## How to run server (CMD or PowerShell)
	Hosted on TCP port 8080
	$ MessagingServer.exe
