#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <vector>
#include <netinet/udp.h>
#include <netinet/ip.h>

/*
Code referenced:
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hala001/sendto.htm
https://www.binarytides.com/raw-udp-sockets-c-linux/
https://www.digi.com/resources/documentation/Digidocs/90002002/Tasks/t_calculate_checksum.htm?TocPath=API%20Operation%7CAPI%20frame%20format%7C_____1
https://gist.github.com/listnukira/4045436 
*/

using namespace std;

//code from binarytides, updated to c++11
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
    char data_sent[256], *pseudogram; // Used for raw socket
    string hello = "SHOW ME WHAT YOU GOT";
    char *udpData;
    char data_recv[256];
    struct sockaddr_in to, my_addr;
    struct sockaddr from;
    struct pseudo_header pHeader;
    vector<int> openPorts;
    struct timeval timeout;
    
    fd_set fdset;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int rawSock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    to.sin_family = AF_INET;

    /*************************PROBLEM 1*************************/
    //loop through the ports, from low to high
    for(int i = portNoLow; i <= portNoHigh; i++) {
        memset(&to, 0, sizeof(to));
        memset(data_recv, 0, sizeof(data_recv));
        //assign the port to the sockaddress
        to.sin_port = htons(i);
        inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);

        //send data
        sendto(sock, hello.c_str(), sizeof(hello), 0, (struct sockaddr*)&to, sizeof(to));
        //cout << "Sending bytes to " << i << ".." << endl;
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        socklen_t addrlen = sizeof(from);
        //assign timeout
        timeout.tv_sec = 1; //this is excessive, but looks better than having tv_usec in the hundreds of thousands.
        timeout.tv_usec = 0;
        if(select(sock+1, &fdset, NULL, NULL, &timeout) > 0)
        // if(select(sock+1, &from, NULL, NULL, &timeout) > 0)
        {
            //if recieved data, print it out and move on
            recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);        
            cout << "Data received from port " << i << ": " << data_recv << endl;
            openPorts.push_back(i);
        } else {
            //timeout or error
        }
        
    }
    
    // Get our address and socket

    char myIP[16];
    unsigned int myPort;

    bzero(&my_addr, sizeof(my_addr));
	socklen_t len = sizeof(my_addr);
	getsockname(sock, (struct sockaddr *) &my_addr, &len);
	inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
	myPort = ntohs(my_addr.sin_port);
    //close the socket after use
    close(sock);

    /*************************PROBLEM 2*************************/
    // After finding open sockets, send UDP packet with appropriate header
    // Loop through open ports, send packets with custom checksum and evil bit
    
    // Hard coded solution for the 2 open ports with hidden ports
    // Port looking for checksum 61453

    // Raw socket header below
    // Fill in IP header
    struct iphdr *ipHeader = (struct iphdr*) data_sent;

    // Fill in UDP header 
    struct udphdr *udpHeader = (struct udphdr*) (data_sent + sizeof(struct ip));

    struct pseudo_header psh;
    udpData = data_sent + sizeof(struct iphdr) + sizeof(struct udphdr); // TODO: comment
    strcpy(udpData, "knock");

    //some address resolution
    

    //to.sin_port = htons();
    //to.sin_addr.s_addr = inet_addr ("192.168.1.1");
     
    //Fill in the IP Header
    ipHeader->ihl = 5;
    ipHeader->version = 4;
    ipHeader->tos = 0;
    ipHeader->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(udpData);
    ipHeader->id = htonl (54321); //Id of this packet
    ipHeader->frag_off = 0;
    ipHeader->ttl = 255;
    ipHeader->protocol = IPPROTO_UDP;
    ipHeader->check = 0;      //Set to 0 before calculating checksum
    ipHeader->saddr = inet_addr ( myIP );    //Spoof the source ip address
    ipHeader->daddr = to.sin_addr.s_addr;
     
    //Ip checksum
    ipHeader->check = chkSum ((unsigned short *) udpData, ipHeader->tot_len);
     
    //UDP header
    udpHeader->source = htons (6666);
    udpHeader->dest = htons (8622);
    udpHeader->len = htons(8 + strlen(udpData)); //tcp header size
    udpHeader->check = 0; //leave checksum 0 now, filled later by pseudo header
     
    //Now the UDP checksum using the pseudo header
    psh.source_address = inet_addr( ipAddress.c_str() );
    psh.dest_address = to.sin_addr.s_addr;
    psh.zeroes = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(struct udphdr) + strlen(udpData) );
     
    int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + strlen(udpData);
    pseudogram = (char*)malloc(psize);
     
    memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header) , udpHeader , sizeof(struct udphdr) + strlen(udpData));
     
    udpHeader->check = chkSum( (unsigned short*) pseudogram , psize);


    memset(&to, 0, sizeof(to));
    memset(data_recv, 0, sizeof(data_recv));
    inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);
    sendto(rawSock, hello.c_str(), sizeof(hello), 0, (struct sockaddr*)&to, sizeof(to));
        //cout << "Sending bytes to " << i << ".." << endl;
    FD_ZERO(&fdset);
    FD_SET(rawSock, &fdset);
    socklen_t addrlen = sizeof(from);
    //assign timeout
    timeout.tv_sec = 1; //this is excessive, but looks better than having tv_usec in the hundreds of thousands.
    timeout.tv_usec = 0;
    if(select(rawSock, &fdset, NULL, NULL, &timeout) > 0)
    {
        //if recieved data, print it out and move on
        recvfrom(rawSock, data_recv, sizeof(data_recv), 0, &from, &addrlen);        
        cout << "Data received from port " << "4041" << ": " << data_recv << endl;
    } else {
        //timeout or error
        cout << "Port not open..." << endl;
    }
    
    close(rawSock);
   

    return 0;
}