#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Invalid arguments\n");
        return 1;
    }
    size_t len;
    sscanf(argv[1], "%zu", &len);



    //CREDIT : recitation
    int socketFd = -1;
    ssize_t bytesRead = 0;
    char inBuffer[1024];

    struct sockaddr_in serv_addr;
    struct sockaddr_in my_addr;
    struct sockaddr_in peer_addr;
    socklen_t addrsize = sizeof(struct sockaddr_in);

    memset(inBuffer, '0', sizeof(inBuffer));
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    getsockname(socketFd, (struct sockaddr *) &my_addr, &addrsize);
    printf("Client: socket created %s:%d\n",
           inet_ntoa((my_addr.sin_addr)),
           ntohs(my_addr.sin_port));

    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(10000); // Note: htons for endiannes
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded...

    printf("Client: connecting...\n");
    if (connect(socketFd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n Error : Connect Failed. %s \n", strerror(errno));
        if (close(socketFd) == -1) {
            printf("\n Error while closing fd %s \n", strerror(errno));
        };
        return 1;
    }
    int outFd = open("/dev/urandom", O_RDONLY);
    if (outFd < 0) {
        printf("\n Error : can't open /dev/urandom. %s \n", strerror(errno));
        if (close(socketFd) == -1) {
            printf("\n Error while closing fd %s \n", strerror(errno));
        };
        return 1;
    }

    size_t totalSent = 0;
    while (totalSent < len) {
        size_t toRead = totalSent + sizeof(inBuffer) < len ? sizeof(inBuffer) : len - totalSent;
        ssize_t readBytes = read(outFd, inBuffer, toRead);
        if (readBytes < 0) {
            printf("\n Error while reading from /dev/urandom. %s \n", strerror(errno));
            if (close(outFd) == -1) {
                printf("\n Error while closing fd %s \n", strerror(errno));
            };
            if (close(socketFd) == -1) {
                printf("\n Error while closing fd %s \n", strerror(errno));
            };
            return 1;
        }
        ssize_t writtenBytes = write(socketFd, inBuffer, (size_t) readBytes);
        if (writtenBytes < 0) {
            printf("\n Error while writing to socket %s \n", strerror(errno));
            if (close(outFd) == -1) {
                printf("\n Error while closing fd %s \n", strerror(errno));
            };
            if (close(socketFd) == -1) {
                printf("\n Error while closing fd %s \n", strerror(errno));
            };
            return 1;
        }
        totalSent += writtenBytes;
    }
    size_t numOfPrintable;

    while (1) {
        bytesRead = read(socketFd, &numOfPrintable, sizeof(numOfPrintable));
        if (bytesRead <= 0) {
            break;
        }
    }
    printf("\n result: %zu\n", numOfPrintable);
    if (close(outFd) == -1) {
        printf("\n Error while closing fd %s \n", strerror(errno));
    };
    if (close(socketFd) == -1) {
        printf("\n Error while closing fd %s \n", strerror(errno));
    };

    exit(0);
}