#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <vector>   // for vectors
#include <regex>    // for regex matching
#include <cstddef>  // for size_t

/*
Code referenced:
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hala001/sendto.htm
https://www.binarytides.com/raw-udp-sockets-c-linux/ 
https://github.com/seifzadeh/c-network-programming-best-snipts/blob/master/Programming%20raw%20udp%20sockets%20in%20C%20on%20Linux
*/

using namespace std;

//code from binarytides and github/seifzadeh
unsigned short chkSum(unsigned short *ptr,int nbytes) 
    {
        unsigned long sum;
        unsigned short oddbyte;
        unsigned short answer;
    
        sum = 0;
        while(nbytes > 1) {
            sum += *ptr++;
            nbytes -= 2;
        }
        if(nbytes == 1) {
            oddbyte = 0;
            *((u_char*) &oddbyte) =* (u_char*) ptr;
            sum += oddbyte;
        }
    
        sum = (sum >> 16)+(sum & 0xffff);
        sum = sum + (sum >> 16);
        answer = (short) ~sum;
        
        return (answer);
    }

int main(int argc, char const *argv[]) {
    // System inputs
    string ipAddress = argv[1]; //"130.208.243.61";
    int portNoLow = atoi(argv[2]); //from 4000
    int portNoHigh = atoi(argv[3]); //to 4100

    struct pseudo_header
    { //IPv4 pseudo header format
        u_int32_t source_address; //both the source address and destination address are 32 bits
        u_int32_t dest_address;
        u_int8_t zeroes;
        u_int8_t protocol;
        u_int16_t udp_length;
    };

    unsigned short checksum = 61453; //hard coded solution for now

    // IBM Code
    int bytes_sent;
    int bytes_received;
    char data_sent[256];
    string hello = "SHOW ME WHAT YOU GOT"; 
    char data_recv[256];
    struct sockaddr_in to;
    struct sockaddr from;

    vector<int> openPorts;
    int portsFound[3];
    int secretPorts[3];
    struct timeval timeout;

    fd_set fdset;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int rawSock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    to.sin_family = AF_INET;
    //loop through the ports, from low to high
    for(int i = portNoLow; i <= portNoHigh; i++) {
        memset(&to, 0, sizeof(to));
        memset(data_recv, 0, sizeof(data_recv));
        //assign the port to the sockaddress
        to.sin_port = htons(i);
        inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);

        //send data
        bytes_sent = sendto(sock, hello.c_str(), sizeof(hello), 0, (struct sockaddr*)&to, sizeof(to));
        //cout << "Sending bytes to " << i << ".." << endl;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        socklen_t addrlen = sizeof(from);
        //assign timeout
        timeout.tv_sec = 1; //this is excessive, but looks better than having tv_usec in the hundreds of thousands.
        timeout.tv_usec = 0;
        if(select(sock+1, &fdset, NULL, NULL, &timeout) > 0)
        {
            //if recieved data, print it out and move on
            bytes_received = recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);
              
            cout << "Port " << i << ": " << data_recv << endl;
            if(regex_match (data_recv, regex("(Please send)(.*)+")))
            {
                cout << "Found the checksum!" << endl;
                portsFound[0] = i;
            }
            else if(regex_match(data_recv, regex("(I only speak)(.*)+")))
            {
                cout << "Found the evil corporation!" << endl;
                portsFound[1] = i;
            }
            else if(regex_match(data_recv, regex("(This is the port)(.*)+")))
            {
                string secretPort = data_recv;
                size_t end = secretPort.find_last_of(' ');
                cout << "Found secret port, it's at: " << secretPort.substr(end+1) << endl;
                secretPorts[2] = atoi(secretPort.substr(end+1).c_str());
            }
            else
            {
                cout << "Found the oracle!" << endl;
                portsFound[2] = i;
            }
            //openPorts.push_back(i);
        } else {
            //timeout or error
        }
        
    }
    //close the socket after use
    close(sock);

    // After finding open sockets, send UDP packet with appropriate header
    // Loop through open ports, send packets with custom checksum and evil bit
    
    // Hard coded solution for the 2 open ports with hidden ports
    // Port looking for checksum 61453

    to.sin_port = htons(4041);
    memset(&to, 0, sizeof(to));
    memset(data_recv, 0, sizeof(data_recv));
    inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);
    
    close(rawSock);
   

    return 0;
}