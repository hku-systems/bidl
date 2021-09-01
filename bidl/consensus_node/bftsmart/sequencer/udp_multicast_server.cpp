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

#define SERV_PORT  7777 
const int MAX_LEN = 1024*1024*1024;
char recv_buf[MAX_LEN];
struct ip_mreq        group;
const uint8_t MAGIC[4] = {7,7,7,7};

int main() {
    int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0) {
        perror("socket");
        exit(1);
    }

    /*
     * Enable SO_REUSEADDR to allow multiple instances of this
     * application to receive copies of the multicast datagrams.
     */
    {
        int reuse=1;
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
                    (char *)&reuse, sizeof(reuse)) < 0) {
            perror("setting SO_REUSEADDR");
            close(sock_fd);
            exit(1);
        }
    }

    // int flag =1;
    // setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &flag, sizeof(flag));
    struct sockaddr_in addr_serv;
    int len;
    memset(&addr_serv, 0, sizeof(struct sockaddr_in));
    addr_serv.sin_family = PF_INET;
    addr_serv.sin_port = htons(SERV_PORT);
    addr_serv.sin_addr.s_addr = htonl(INADDR_ANY);
    // addr_serv.sin_addr.s_addr = inet_addr("10.22.1.6");
    len = sizeof(addr_serv);

    if(bind(sock_fd, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0) {
        perror("bind error:");
        exit(1);
    }

    /*
     * Join the multicast group 231.0.0.0 on the local 10.22.1.8
     * interface.  Note that this IP_ADD_MEMBERSHIP option must be
     * called for each local interface over which the multicast
     * datagrams are to be received.
     */
    group.imr_multiaddr.s_addr = inet_addr("231.0.0.0");
    group.imr_interface.s_addr = inet_addr("10.22.1.8");
    if (setsockopt(sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                (char *)&group, sizeof(group)) < 0) {
        perror("adding multicast group");
        close(sock_fd);
        exit(1);
    }

    int recv_num;
    int send_num;
    struct sockaddr_in addr_client;
    int tot = 0;
    int prtInterval = 10000;
    typedef chrono::high_resolution_clock Clock;
    auto t1 = Clock::now();
    cout << "server wait:"<< endl;
    while(1) {
        recv_num = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);
        if(recv_num < 0) {
            perror("recvfrom error:");
            exit(1);
        }
        recv_buf[recv_num] = '\0';
        // printf("server receive %d bytes", recv_num);
        // the first 4 bytes are the MAGIC Number "7777"
        uint64_t *x = (uint64_t*)(recv_buf + 4);
        tot++;
        if(tot % prtInterval == 0) {
            auto t2 = Clock::now();
            auto interval = chrono::duration_cast<chrono::milliseconds>(t2-t1).count() / 1000.0;
            t1 = t2;
            // cout << interval  << " tps:" << prtInterval / interval << endl;
            cout << "Total: " << tot << " Interval: " << interval  << " TPS:" << prtInterval / interval << endl;
        }
        printf("seq: %ld\n", *x);
    }
    close(sock_fd);
    return 0;
}
