#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>


int main(int argc, char **argv) {
    if (argc != 5) {
        printf("Invalid args");
        return -1;
    }
    size_t cnt = 0;
    char targetChar = argv[1][0];
    char fileName[256];
//    char pipeName[256] = "/tmp/counter_";
    char* pipeName = "/tmp/counter_";
    strcpy(fileName, argv[2]);
    off_t offset = (off_t) argv[3];
    size_t length = (size_t) argv[4];
    int fd = open(fileName, O_RDONLY);
    lseek(fd, offset, SEEK_SET);
    // CREDIT mmap: recitation 5
    char *arr = (char *) mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);
    if (arr == MAP_FAILED) {
        printf("Error mmapping the file: %s\n", strerror(errno));
        return -1;
    }

    for (size_t i = 0; i < length; i++) {
        if (arr[i] == targetChar) {
            cnt++;
        }
    }

    pid_t myPid = getpid();
//    strcat(pipeName, myPid);
    pipeName += sprintf(pipeName, "%ld", myPid);
    mkfifo(pipeName, 0666);
    int outFd = open(pipeName, 0444);
    pid_t dispatcherPid = getppid();
    kill(dispatcherPid, SIGUSR1);
    write(outFd, &cnt, sizeof(cnt));
    sleep(1);
    if (munmap(arr, length) == -1) {
        printf("Error un-mmapping the file: %s\n", strerror(errno));
        return -1;
    }
    close(fd);
    close(outFd);
    unlink(pipeName);
    exit(0);
}
