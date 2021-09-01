/**
 * A software-implementation of sequencer:
 *  1. Receive UDP packet at 8888 port.
 *  2. Insert sequence number(4 bytes) in front of the packet body.
 *  3. Broadcast the processed packet to the group address 10.22.1.255:9999.
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
using namespace std;

struct in_addr localInterface;
const int SERV_PORT = 8888;
const int BROA_PORT = 7777;
const int BUF_LEN = 1024*1024*1024;
uint8_t recv_buf[BUF_LEN];

int main() {
    int recv_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(recv_fd < 0) {
        perror("socket");
        exit(1);
    }

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
    cout << "addr length:"  << len << endl;

    int recv_num;
    int send_num;
    struct sockaddr_in addr_client;
    
    /*
    * Declare the socket used for udp multicast
    */
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
    localInterface.s_addr = inet_addr("10.22.1.8");
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

    uint32_t seq = 0;
    int tot = 0; // number of received transactions
    typedef chrono::high_resolution_clock Clock;
    auto t1 = Clock::now();
    while(1) {
        // printf("wait\n");
        recv_num = recvfrom(recv_fd, recv_buf+4, sizeof(recv_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
        if(recv_num < 0) {
            perror("recvfrom error:");
            exit(1);
        } else {
            // printf("recv: %d\n", recv_num);
            char* ipString = inet_ntoa(addr_client.sin_addr);
            uint64_t *seq = (uint64_t*)(recv_buf+4);
            // printf("%s %d %ld\n", ipString, ntohs(addr_client.sin_port), *seq);
            // cout << "IP Address:" << ipString << ", Port:" << ntohs(addr_client.sin_port) << ", Seq Num:" << unsigned(*seq) << endl;
        }

        // assign seq number
        uint32_t *ptr = (uint32_t*)recv_buf;
        *ptr = seq++;

        send_num = sendto(send_fd, recv_buf, recv_num+4, 0, (struct sockaddr *)&multicast_addr, socklen);
        if(send_num < 0) {
            perror("sendto error:");
            exit(1);
        } 
        tot++;
        int prtInterval= 10000;
        if(tot % prtInterval == 0) {
            auto t2 = Clock::now();
            auto interval = chrono::duration_cast<chrono::milliseconds>(t2-t1).count() / 1000.0;
            t1 = t2;
            cout << "Total: " << tot << " Interval: " << interval  << " TPS:" << prtInterval / interval << endl;
        }
    }
    close(recv_fd);
    close(send_fd);
    return 0;
}
