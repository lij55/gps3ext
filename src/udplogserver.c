#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>

#define BUFSIZE 1024

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

int createsocket(const char* addr) {
    if(!addr || !addr[0]) {
        fprintf(stderr, "empty listening address\n");
        return -1;
    }

    int ret, sockfd = -1;
    struct sockaddr_un address;

    sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sockfd == -1) goto error;
    
    if(-1 == unlink(addr)) {
        perror("delete file fail");
    }

    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, addr, UNIX_PATH_MAX-1);

    if(-1 == bind(sockfd, (struct sockaddr *) &address, 
                  sizeof(struct sockaddr_un))) {
        goto error;
    }
    return sockfd;
 error:
    perror("Create Socket error");
    if(sockfd != -1)
        close(sockfd);
    return -1;
}
                         
int main(int argc, char* argv[]) {
    if(argc < 2) {
        fprintf(stderr, "not enough parameters\n");
        return 1;
    }
    int fd = createsocket(argv[1]);

    if ( fd == -1 ) {
        return 1;
    }

    int bytes_received;
    char buf[BUFSIZE];
    while(1) {
        bytes_received = recv(fd, buf, BUFSIZE, 0);
        if(bytes_received == -1) {
            perror("receive data fail");
            break;
        }
        if(bytes_received == 0) {
            break;
        }
        fprintf(stdout, "%.*s", bytes_received, buf);
    }
    
    unlink(argv[1]);
    close(fd);
    return 0;
}
