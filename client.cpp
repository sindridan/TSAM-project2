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
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    // IBM Code
    int bytes_sent;
    int bytes_received;
    char data_sent[256];
    char data_recv[256];
    struct sockaddr_in to;
    struct sockaddr from;

    struct timeval timeout;

    timeout.tv_sec = 2;
    timeout.tv_usec = 0;    
    
    to.sin_family = AF_INET;
    inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);

    for(int i = portNoLow; i <= portNoHigh; i++) {
        memset(&to, 0, sizeof(to));
        to.sin_port = htons(i);
        cout << "Port no: " << i << endl;
        //IBM code
        cout << "Sending bytes to " << i << ".." << endl;
        bytes_sent = sendto(sock, data_sent, sizeof(data_sent), 0, (struct sockaddr*)&to, sizeof(to));
        socklen_t addrlen = sizeof(from);
        cout << "Size of addrlen " << addrlen << ".." << endl;
        cout << "Now receiving bytes from " << i << ".." << endl;
        
        
        bytes_received = recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);
        cout << "Bytes received from " << i << ".." << endl;

        if (sizeof(data_recv) > 0) {
            cout << "Port no: " << i << " sent back data!";
        } else {
            cout << "Port no: " << i << " sent no data back.....";
        }
    }
    
    return 0;
}