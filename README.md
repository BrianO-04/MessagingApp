# MessagingApp

Self hosted direct messaging app written in C using sockets and the TCP protocol

## How to compile (Linux) 
	$ git clone https://github.com/BrianO-04/MessagingApp.git/
	$ cd MessagingApp	
	$ cmake --build build
## How to run client (Linux)
	$ cd build
	$ ./MessagingApp {Name} {IP}
## How to run server (Linux)
	Hosted on TCP port 8080
	$ cd build
	$ ./MessagingServer
