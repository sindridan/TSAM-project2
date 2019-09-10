all: client

client: client.cpp
	g++ -Wall -std=c++11 client.cpp -o client
