#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;

string SERVER_IP;
const int SERVER_PORT = 8888;
const int BUF_LEN = 1024*1024*1024;
char buf[BUF_LEN];

void udp_msg_sender(int fd, struct sockaddr* dst) {
    socklen_t len;
    struct sockaddr_in src;
    cout << "Please input the message size:" << endl;
    int size;
    cin >> size;
    char* msg = new char[size];
    cout << "Please input the number of messages to be sent:" << endl;
    int n;
    cin >> n;
    while(n--) {
        len = sizeof(*dst);
        // cout << "Message length is:" << s.c_str() << endl;
        sendto(fd, msg, size, 0, dst, len);
    }
}

int main(int argc, char* argv[]) {
    SERVER_IP = argv[1];
    int client_fd;
    struct sockaddr_in ser_addr;

    client_fd = socket(PF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0) {
        printf("create socket fail!\n");
        return -1;
    }

    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = PF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP.c_str());
    // ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
    ser_addr.sin_port = htons(SERVER_PORT); 

    udp_msg_sender(client_fd, (struct sockaddr*)&ser_addr);

    close(client_fd);
    return 0;
}
