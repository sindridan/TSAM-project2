#define _XOPEN_SOURCE_EXTEND 1
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
#include <netinet/udp.h>
#include <netinet/ip.h>

/*
Code referenced:
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hala001/sendto.htm
https://www.binarytides.com/raw-udp-sockets-c-linux/
https://www.digi.com/resources/documentation/Digidocs/90002002/Tasks/t_calculate_checksum.htm?TocPath=API%20Operation%7CAPI%20frame%20format%7C_____1
https://gist.github.com/listnukira/4045436
https://beej.us/guide/bgnet/html/multi/inet_ntopman.html 
*/

using namespace std;

//code from binarytides and github/seifzadeh
unsigned short chkSum(unsigned short *ptr,int nbytes) 
    {
        //cout << "In data: " << *ptr << " and " << nbytes << endl;
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
        //cout << "Answer: "<< answer << endl;
        return (answer);
    }

    char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) 
    {
        switch(sa->sa_family) {
            case AF_INET:
                inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                        s, maxlen);
                break;

            case AF_INET6:
                inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                        s, maxlen);
                break;

            default:
                strncpy(s, "Unknown AF", maxlen);
                return NULL;
        }

        return s;
    }

int main(int argc, char const *argv[]) {
    // System inputs
    string ipAddress = argv[1]; //"130.208.243.61";
    int portNoLow = atoi(argv[2]); //from 4000
    int portNoHigh = atoi(argv[3]); //to 4100


    struct pseudo_header
    {
        u_int32_t source_address =  inet_addr( "10.2.26.121" );
        u_int32_t dest_address;
        u_int8_t placeholder;
        u_int8_t protocol;
        u_int16_t udp_length;
    };
 
    struct udpHeader
    {
        uint16_t source; //both the source address and destination address are 32 bits;
        uint16_t dest;
        uint16_t len;
        uint16_t check;
        uint32_t data;
    };

    //unsigned short checksum = 61453; //hard coded solution for now

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
    int portsFound[3];
    int secretPorts[3];
    unsigned short desiredChkSum;
    struct timeval timeout;
    //unsigned short payload = 4;
    int opt = 1;
    fd_set fdset;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock == -1) {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create raw socket");
    }
    int rawSock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(rawSock == -1) {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create raw socket");
    } else { cout << "Raw socket successfully created!" << endl; }

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
        FD_ZERO(&fdset);
        FD_SET(sock, &fdset);
        socklen_t addrlen = sizeof(from);
        
        //assign timeout
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        if(select(sock+1, &fdset, NULL, NULL, &timeout) > 0)
        {
            //if recieved data, print it out and move on
            char str[INET_ADDRSTRLEN];
            bytes_received = recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);
            cout << get_ip_str(&from, str, 20) << endl;
            cout << "Port " << i << ": " << data_recv << endl;
            if(regex_match (data_recv, regex("(Please send)(.*)+")))
            {
                cout << "Found the checksum!" << endl;
                string temp = data_recv;
                size_t end = temp.find_last_of(' ');
                cout << "desired checksum is: " << temp.substr(end+1) << endl;
                desiredChkSum = atoi(temp.substr(end+1).c_str());
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
                size_t end = secretPort.find_last_of(':');
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
    
    // Get our address and socket
    char myIP[16];
    unsigned int myPort;

    bzero(&my_addr, sizeof(my_addr));
	socklen_t len = sizeof(my_addr);
	getsockname(sock, (struct sockaddr *) &my_addr, &len);
	//inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
	myPort = ntohs(my_addr.sin_port);
    /*
    for(int i = 0; i < 3; i++){
        cout << "Found port: " << portsFound[i] << endl;
    }
    for(int i = 0; i < 3; i++){
        cout << "Found port: " << secretPorts[i] << endl;
    }
    printf("My ip address is: %s\n", myIP);
    cout << "My port is: " << myPort << endl;
    */

    //close the socket after use
    /*************************PROBLEM 2 - Easy given Port*************************/

    /*************************PROBLEM 2 - Evil Bit*************************/
    /*cout << "Problem 2: evil bit!" << endl;
    int evilSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(evilSock == -1) {
        //socket creation failed, may be because of non-root privileges
        perror("Failed to create evil socket");
    }
    
    // Fill in IP header
    struct iphdr *ipHeader = (struct iphdr*) data_sent;

    // Fill in UDP header 
    struct udphdr *udpHeader = (struct udphdr*) (data_sent + sizeof(struct ip));

    struct pseudo_header psh;
    udpData = data_sent + sizeof(struct iphdr) + sizeof(struct udphdr); // TODO: comment
    string knock = "knock";
    strcpy(udpData, knock.c_str());

    //Fill in the IP Header
    ipHeader->ihl = 5;
    ipHeader->version = 4;
    ipHeader->tos = 0;
    ipHeader->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + strlen(udpData);
    ipHeader->id = htonl (54321); //Id of this packet
    ipHeader->frag_off = htons (IP_RF); // for evil bit
    ipHeader->ttl = 255;
    ipHeader->protocol = IPPROTO_UDP;
    ipHeader->check = 0;      //Set to 0 before calculating checksum
    ipHeader->saddr = inet_addr ( myIP );    //Spoof the source ip address
    ipHeader->daddr = to.sin_addr.s_addr;
    
    //Ip checksum
    ipHeader->check = chkSum ((unsigned short *) udpData, ipHeader->tot_len);
    
    //UDP header
    udpHeader->source = htons (myPort);
    udpHeader->dest = htons (portsFound[1]);
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


    // memset(&to, 0, sizeof(to));
    memset(data_recv, 0, sizeof(data_recv));
    // inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);
    sendto(rawSock, data_sent, ipHeader->tot_len, 0, (struct sockaddr*)&to, sizeof(to));
    
    //cout << "Sending bytes to " << portsFound[0] << ".." << endl;

    socklen_t addrlen = sizeof(from);
    //assign timeout
    timeout.tv_sec = 1; //this is excessive, but looks better than having tv_usec in the hundreds of thousands.
    timeout.tv_usec = 0;

    memset(&to, 0, sizeof(to));
    //assign the port to the sockaddress
    to.sin_port = htons(myPort);
    inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);

    //cout << "Sending bytes to " << i << ".." << endl;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);

    if(select(sock +1, &fdset, NULL, NULL, &timeout) > 0)
    {
        //if recieved data, print it out and move on
        recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);        
        cout << "Data received "<< ": " << data_recv << endl;
    } else {
        //timeout or error
        cout << "Port not open..." << endl;
    }    
    
    
    close(rawSock);
    
    */
    /*************************PROBLEM 2 - Checksum Port*************************/
    // After finding open sockets, send UDP packet with appropriate header
    // Loop through open ports, send packets with custom checksum and evil bit
    
    struct iphdr ipHeader;
    struct udpHeader udpHeader;
    struct pseudo_header psh;

    char datagram[1024];
    memset (datagram, 0, 1024);


    // Port looking for checksum 61453
    if(setsockopt(rawSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
    }

    //Fill in the IP Header
    ipHeader.ihl = 5;
    ipHeader.version = 4;
    ipHeader.tos = 0;
    ipHeader.tot_len = sizeof (struct iphdr) + sizeof (struct udpHeader);
    ipHeader.id = htonl (54321); //Id of this packet
    ipHeader.frag_off = 0; // for evil bit
    ipHeader.ttl = 255;
    ipHeader.protocol = IPPROTO_UDP;
    ipHeader.check = 0;      //Set to 0 before calculating checksum
    ipHeader.saddr = inet_addr( "10.2.26.121" );    //Spoof the source ip address
    ipHeader.daddr = to.sin_addr.s_addr;

    ipHeader.check = chkSum ((unsigned short *) datagram, ipHeader.tot_len);
    
    //UDP header
    udpHeader.source = htons(myPort);
    udpHeader.dest = htons(portsFound[0]);
    udpHeader.len = htons(24); //tcp header size
    udpHeader.check = 0;
    udpHeader.data = 57885; // hardcoded for checksum to work
    
    // psh.source_address;
    psh.dest_address = to.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = htons(sizeof(udpHeader));

    int psize = sizeof(psh) + sizeof(udpHeader);

    memset(data_sent, 0, sizeof(data_sent));
    memcpy(data_sent , &psh , sizeof(psh));
    memcpy(data_sent + sizeof (psh) , &udpHeader , sizeof (udpHeader));

    udpHeader.check = chkSum( (unsigned short*) data_sent , sizeof(psh) + sizeof(udpHeader));
    udpHeader.data = -(htons((unsigned short)desiredChkSum) - udpHeader.check);
    udpHeader.check = 0;

    memset(data_sent, 0, sizeof(data_sent));
    memcpy(data_sent , &psh , sizeof(psh));
    memcpy(data_sent + sizeof (psh) , &udpHeader , sizeof (udpHeader));

    udpHeader.check = chkSum( (unsigned short*) data_sent , sizeof(psh) + sizeof(udpHeader));

    cout << "Last checksum function: " << udpHeader.check << endl;
    memset(data_sent, 0, sizeof(data_sent));
    memcpy(data_sent , &psh , sizeof(psh));
    memcpy(data_sent + sizeof (psh) , &udpHeader , sizeof (udpHeader));

    for(int i = 0; i < 24; i++) {
        printf("%02x ", (unsigned char)data_sent[i]);
    } cout << endl;

    if(sendto(rawSock, data_sent, ipHeader.tot_len, 0, (struct sockaddr*)&to, sizeof(to)) < 0){
        cout << "sendTo failed" << endl;
    }

    socklen_t addrlen = sizeof(from);
    //assign timeout
    timeout.tv_sec = 1; //this is excessive, but looks better than having tv_usec in the hundreds of thousands.
    timeout.tv_usec = 0;

    memset(&to, 0, sizeof(to));
    //assign the port to the sockaddress
    to.sin_port = htons(myPort);
    inet_pton(AF_INET, ipAddress.c_str(), &to.sin_addr);

    //cout << "Sending bytes to " << i << ".." << endl;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);

    if(select(sock +1, &fdset, NULL, NULL, &timeout) > 0)
    {
        //if recieved data, print it out and move on
        recvfrom(sock, data_recv, sizeof(data_recv), 0, &from, &addrlen);        
        cout << "Data received "<< ": " << data_recv << endl;
    } else {
        //timeout or error
        cout << "Port not open..." << endl;
    }
    
    close(sock);

    return 0;
}