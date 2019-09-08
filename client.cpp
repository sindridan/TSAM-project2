#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

/*
Code referenced:
https://gist.github.com/codehoose/d7dea7010d041d52fb0f59cbe3826036#file-bbclient-cpp-L42
TSAM slides, lecture 1
*/

using namespace std;
int main(int argc, char const *argv[]) {
    
    //Hint structure
    string ipAddress = argv[1]; //"130.208.243.61";
    int portNo = atoi(argv[2]); //4021;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(portNo);

    //converting ipaddress to series of bytes and put into buffer, creating hint structure
    inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

    //int conn = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if(connect(sock, (struct sockaddr *)&hint, sizeof(hint)) < 0) {
        perror("Failed to open socket");
        return(-1);
    }

    char buffer[1025];
    string inp;
    cout << "Enter q to quit message loop!" << endl;
    do{
        cout << "msg to server: ";
        getline(cin, inp);
        if(inp == "q" || inp == "Q") {
            break;
        } 
        int sendResults = send(sock, inp.c_str(), inp.size(), 0);
        memset(buffer, 0, 1025);
        int received = recv(sock, buffer, 1025, 0);
        
        cout << "From SERVER: " << string(buffer, received) << endl;
        
    }while(true);

    close(sock);

    return 0;
}