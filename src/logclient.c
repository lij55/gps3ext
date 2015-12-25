#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

#define TESTADDR "/tmp/testsocket"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

int threadnum = 5;
int msgnum = 102400;

const char* TESTSTR =  "SDAFASFASFADFGADSFASFADSFASFS\n";

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789-=!@#$%^&*()_+\\|"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    int i = 0;
    int index = 0;
    for (i = 0; i < len; ++i) {
        index = rand() % (sizeof(alphanum) - 1); // trailing \0
        s[i] = alphanum[index];
        // fprintf(stderr, "%d:%c ", index, s[i]);
    }
    // fprintf(stderr, "\n%d\t%.*s\n", sizeof(alphanum), len, s);
}

void* workerthread(void* unused) {
    int socket_fd = -1;
    struct sockaddr_un address;
    char buffer[1024];

    socket_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if(socket_fd < 0) {
        perror("create socket fail");
        return NULL;
    }
    
    /* start with a clean address structure */
    memset(&address, 0, sizeof(struct sockaddr_un));
 
    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, TESTADDR);
    int i;
    for ( i = 0; i < msgnum; i++) {
        gen_random(buffer, 1024);
#if 1
        sendto(socket_fd, buffer, 1024, 0,
               (struct sockaddr *) &address, 
           sizeof(struct sockaddr_un));
#endif 
    }
    close(socket_fd);
    return NULL;
}

int main (int argc, char* argv[]) {
    int randomnum;
    int fd = open("/dev/urandom", O_RDONLY);
    if(fd == -1) {
        srand(time(NULL));
    } else {
        read(fd, (char*) &randomnum, 4);
        srand(randomnum);
        close(fd);
    }

    pthread_t threads[threadnum];

    int i;
    for(i = 0; i < threadnum; i++) {
        pthread_create(&threads[i], NULL, workerthread, NULL);
    }
    
    for(i = 0; i < threadnum; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
