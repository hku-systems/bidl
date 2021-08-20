#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define SERV_PORT  7777 
const int MAX_LEN = 1024*1024*1024;
char recv_buf[MAX_LEN];

int main() {
    int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock_fd < 0) {
        perror("socket");
        exit(1);
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


    int recv_num;
    int send_num;
    struct sockaddr_in addr_client;

    while(1) {
      printf("server wait:\n");

      recv_num = recvfrom(sock_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&addr_client, (socklen_t *)&len);

      if(recv_num < 0) {
        perror("recvfrom error:");
        exit(1);
      }

      recv_buf[recv_num] = '\0';
      printf("server receive %d bytes, content: %s\n", recv_num, recv_buf+4);
      uint64_t *x = (uint64_t*)recv_buf;
      printf("seq: %d\n", *x);
    }

    close(sock_fd);
    return 0;
}
