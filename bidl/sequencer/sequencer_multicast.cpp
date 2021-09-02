/**
 * A software-implementation of sequencer:
 *  1. Receive UDP packet at 8888 port.
 *  2. Insert sequence number(4 bytes) in front of the packet body.
 *  3. Insert magic number(4 bytes) in front of the packet body.
 *  4. Multicast the processed packet to the group address 231.0.0.0:7777.
 */
#include <stdio.h>
#include <sys/types.h>
#include <chrono>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <net/if.h>
#include <sys/ioctl.h>

using namespace std;

struct in_addr localInterface;
const int SERV_PORT = 8888;
const int BROA_PORT = 7777;
const int BUF_LEN = 1024*1024*1024;
uint8_t recv_buf[BUF_LEN];
const uint8_t MAGIC[4] = {7,7,7,7};

void Print(uint8_t str[], uint64_t n)
{
	uint64_t i = 0;
	for (i = 0; i<n; i++)
	{
		printf("%d ", str[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[]) {
    /*
    * Multicast interval
    */
    int TPS = 0;
    if( argc == 2) {
        TPS = atoi(argv[1]);
        cout << "Multicast TPS:" << TPS << endl;
    } else if( argc > 2) {
        cout << "Usage: ./sequencer TPS" << endl;
        return 1;
    } else {
        cout << "Usage: ./sequencer TPS" << endl;
        return 1;
    }
    int send_interval = 1e6 / TPS / 1e3;
    chrono::microseconds sleep_duration{send_interval};
    cout << "Multicast interval:" << sleep_duration.count() << endl;
    cout << "Group Address:" << "231.0.0.0" << endl;

    /*
    * Declare the socket used for receiving transactions from clients
    */
    int recv_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(recv_fd < 0) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(recv_fd, SOL_SOCKET, SO_RCVBUF, &BUF_LEN, sizeof(BUF_LEN)) == -1) {
        cout<< "Error setting udp receive buffer" <<endl;
    }

    /*
    * Declare the socket used for udp multicast
    */
    struct sockaddr_in addr_serv;
    int len;
    memset(&addr_serv, 0, sizeof(struct sockaddr_in));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(SERV_PORT);
    addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);
    len = sizeof(addr_serv);

    if(bind(recv_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0) {
        perror("bind error:");
        exit(1);
    }

    int recv_num;
    int send_num;
    struct sockaddr_in addr_client;
    
    int send_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if(send_fd < 0) {
        perror("socket");
        exit(1);
    }

    /*
    * Set local interface for outbound multicast datagrams.
    * The IP address specified must be associated with a local,
    * multicast-capable interface.
    */
    int fd;
	struct ifreq ifr;

    char iface[] = "enp5s0";
    fd = socket(AF_INET, SOCK_DGRAM, 0);

	//Type of address to retrieve - IPv4 IP address
	ifr.ifr_addr.sa_family = AF_INET;

	//Copy the interface name in the ifreq structure
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
	ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);

	//display result
	cout << "Binded to NIC: " << iface << "--" << inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr)<< endl;
    string interface_addr = inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);

    // localInterface.s_addr = inet_addr("10.22.1.7");
    localInterface.s_addr = inet_addr(interface_addr.c_str());
    if (setsockopt(send_fd, IPPROTO_IP, IP_MULTICAST_IF,
                (char *)&localInterface,
                sizeof(localInterface)) < 0) {
        perror("setting local interface");
        exit(1);
    }

    /*
    * Multicast address
    */
    struct sockaddr_in multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr("231.0.0.0");
    multicast_addr.sin_port = htons(BROA_PORT);
    socklen_t socklen = sizeof(multicast_addr);
    printf("Multicast Address is 231.0.0.0");

    /*
    * Start multicast
    */
    uint32_t seq = 0;
    int tot = 0; // number of received transactions
    typedef chrono::high_resolution_clock Clock;
    auto t1 = Clock::now();
    auto now = Clock::now();
    while(1) {
        // printf("wait\n");
        recv_num = recvfrom(recv_fd, recv_buf+12, sizeof(recv_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
        if(recv_num < 0) {
            perror("recvfrom error:");
            exit(1);
        } else {
            char* ipString = inet_ntoa(addr_client.sin_addr);
        }

        // assign seq number
        uint64_t *ptr = (uint64_t*)(recv_buf + 4);
        *ptr = seq++;

        // assign a magic number "7777" to each transaction
        recv_buf[0] = MAGIC[0];
        recv_buf[1] = MAGIC[1];
        recv_buf[2] = MAGIC[2];
        recv_buf[3] = MAGIC[3];

        // printf("sequencer sends %d bytes\n", recv_num + 12);
        // Print(recv_buf, recv_num+12);

        // recv_num is added with a sequence number (uint64_t) and a magic number (uint32_t)
        send_num = sendto(send_fd, recv_buf, recv_num + 12, 0, (struct sockaddr *)&multicast_addr, socklen);
        if(send_num < 0) {
            perror("sendto error:");
            exit(1);
        } 
        tot++;
        int prtInterval= 1000;
        if(tot % prtInterval == 0) {
            auto t2 = Clock::now();
            auto interval = chrono::duration_cast<chrono::milliseconds>(t2-t1).count();
            t1 = t2;
            cout << "Total " << tot << " Interval " << interval  << " TPS " << prtInterval / interval << " kTxns/s" << endl;
        }
        while(true){
            auto elapsed = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - now);
            if (elapsed > sleep_duration) {
                now = Clock::now();
                break;
            }
        }
    }
    close(recv_fd);
    close(send_fd);
    return 0;
}
