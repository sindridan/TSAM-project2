README for project 1
Author: Sindri Dan Garðarsson, sindrig17@ru.is


HOW TO RUN:
1. Compile the code by running the command 'make' in terminal, where both the server.cpp and client.cpp files is located.
    1.1 the Makefile will compile both the code files in c++11 and create executable files, server and client
2. To run executable server, enter the following commands in the terminal where the server was compiled: './server <port number>'
3. To run executable client, enter the following commands in the terminal (new window) where the client was compiled: './client <ip address> <port number>'
4. In the client, type in SYS <linux terminal command> and the server will execute the command and send back to the client the output
	4.1 If the command doesnt return any output, e.g. mkdir <dir>, then the command will be returned to the client.


Code referenced:

Client.cpp:
https://gist.github.com/codehoose/d7dea7010d041d52fb0f59cbe3826036#file-bbclient-cpp-L42
TSAM slides, lecture 1

Server.cpp:
https://pubs.opengroup.org/onlinepubs/009695399/functions/popen.html

