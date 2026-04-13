# MessagingApp

Self hosted direct messaging app written in C using sockets and the TCP protocol

## How to compile 
	$ git clone https://github.com/BrianO-04/MessagingApp.git/
	$ cd MessagingApp	
	$ mkdir build
	$ cmake .S . -B
	$ cmake --build build
## How to run client
	$ cd build
	$ ./MessagingApp {Name} {IP}
## How to run server
	Hosted on TCP port 8080
	$ cd build
	$ ./MessagingServer
