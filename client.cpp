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
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hala001/sendto.htm
*/

using namespace std;
int main(int argc, char const *argv[]) {
    
    string ipAddress = argv[1]; //"130.208.243.61";
    int portNoLow = atoi(argv[2]); //from 4000
    int portNoHigh = atoi(argv[3]); //to 4100

    // IBM Code
    int bytes_sent;
    int bytes_received;
    char data_sent[256];
    string hello = "Hello from client"; 
    char data_recv[256];
    struct sockaddr_in to;
    struct sockaddr from;

    struct timeval timeout;

    fd_set fdset;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    /*
    int listenSock;
    int clientSock;
    fd_set openSockets;
    fd_set readSockets;
    fd_set exceptSockets;
    int maxfds;
    */

    to.sin_family = AF_INET;
    //loop through the ports, from low to high
    for(int i = portNoLow; i <= portNoHigh; i++) {
        memset(&to, 0, sizeof(to));
        memset(data_recv, 0, sizeof(data_recv));
        //assign the port to the sockaddress
        to.sin_port = htons(i);
        inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);
        //cout << "Port no: " << i << endl;

        //bind(sock, (struct sockaddr*)&to, sizeof(to));
        //send data
        bytes_sent = sendto(sock, hello.c_str(), sizeof(hello), 0, (struct sockaddr*)&to, sizeof(to));
        //cout << "Sending bytes to " << i << ".." << endl;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        socklen_t addrlen = sizeof(from);
        //assign timeout
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        if(select(sock+1, &fdset, NULL, NULL, &timeout) > 0)
        // if(select(sock+1, &from, NULL, NULL, &timeout) > 0)
        {
            //if recieved data, print it out and move on
            bytes_received = recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);        
            cout << "Data received from port " << i << ": " << data_recv << endl;
        } else {
            //timeout or error
        }
    }
    //close the socket after use
    close(sock);
    return 0;
}