#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct sockaddr_in    localInterface;
struct sockaddr_in    group_addr;
int                   sd;
const char*           send_buff = "hello";
char                  loopch = 0;
struct sockaddr_in    localSock;
struct ip_mreq        group;
int                   reuse=1;

// (1) allow multiple processes receive data packets using the same local port
// (2) bind socket to INADDR_ANY:5555
// (3) join the multicast group 225.1.1.1 on the local 127.0.0.1 interface
void multicast_setup_recv(int& sd) {
    // (1) allow multiple processes receive data packets using the same local port
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
        perror("setting SO_REUSEADDR");
        close(sd);
        exit(1);
    }

    // (2) bind socket to INADDR_ANY:5555
    memset((char *) &localSock, 0, sizeof(localSock));
    localSock.sin_family = AF_INET;
    localSock.sin_addr.s_addr = INADDR_ANY;
    localSock.sin_port = htons(5555);
    if (bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) {
        perror("binding datagram socket");
        close(sd);
        exit(1);
    }

    // (3) join the multicast group 225.1.1.1 on the local 127.0.0.1 interface
    group.imr_multiaddr.s_addr = inet_addr("225.1.1.1");
    group.imr_interface.s_addr = inet_addr("127.0.0.1");
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
        perror("adding multicast group");
        close(sd);
        exit(1);
    }
}

// (1) set Group IP and Port
// (2) set Interface for sending data packets
// (3) disable loopback data packets
void multicast_setup_send(int& sd) {
    // (1) set Group IP and Port
    memset((char *) &group_addr, 0, sizeof(group_addr));
    group_addr.sin_family = AF_INET;
    group_addr.sin_addr.s_addr = inet_addr("225.1.1.1");
    group_addr.sin_port = htons(5555);

    // (2) Set local interface for outbound multicast datagrams.
    memset(&localInterface, 0, sizeof(localInterface));
    localInterface.sin_family = AF_INET;
    localInterface.sin_addr.s_addr = inet_addr("127.0.0.1");
    localInterface.sin_port = htons(7777);
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0) {
        perror("setting local interface");
        exit(1);
    }

    // (3) Disable loopback data packets
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
        perror("setting IP_MULTICAST_LOOP:");
        close(sd);
        exit(1);
    }
}

int main (int argc, char *argv[])
{
    struct sockaddr_in client_addr;
    socklen_t  socklen;
    char recv_buff[50];

    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("opening datagram socket");
        exit(1);
    }

    multicast_setup_send(sd);

    // multicast_setup_recv(sd);

    // send a message to the multicast group
    if (sendto(sd, send_buff, strlen(send_buff), 0, (struct sockaddr*)& group_addr, sizeof(group_addr)) < 0) {
        perror("sending datagram message");
    }
    printf("sent message\n");

    // receive a message from the multicast group
    // recvfrom(sd, recv_buff, sizeof(recv_buff), 0,(struct sockaddr *)&client_addr, &socklen);
    // if (read(sd, recv_buff, sizeof(recv_buff)) < 0) {
    //     perror("reading datagram message");
    //     close(sd);
    //     exit(1);
    // }

    //printf("recv client from %s:%d, same port : %d \n", (char *)inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), 1);
    // printf("message = %s\n", recv_buff);

    close(sd);
}
