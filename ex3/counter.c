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
    printf("args: %s, %s, %s, %s\n", argv[1], argv[2], argv[3], argv[4]);
    if (argc != 5) {
        printf("Invalid args2");
        return -1;
    }
    size_t cnt = 0;
    char targetChar = argv[1][0];
    char fileName[256];
    char pipeName[256] = "/tmp/counter_";
    strcpy(fileName, argv[2]);
    off_t offset;
    sscanf(argv[3], "%lld", &offset);
    size_t length;
    sscanf(argv[4], "%zu", &length);
    int fd = open(fileName, O_RDONLY);
    lseek(fd, offset, SEEK_SET);
    // CREDIT mmap: recitation 5
    char *arr = (char *) mmap(NULL, length, PROT_READ, MAP_SHARED, fd, 0);
    if (arr == MAP_FAILED) {
        printf("Error mmapping the file: %s\n", strerror(errno));
        return -1;
    }
    size_t i;
    for (i = 0; i < length; i++) {
        if (arr[i] == targetChar) {
            cnt++;
        }
    }

    pid_t myPid = getpid();
    sprintf(pipeName, "%s%d", pipeName, myPid);
    mkfifo(pipeName, 0666);
    printf("%s\n", pipeName);
    int outFd = open(pipeName, O_WRONLY);
    printf("after open\n");
    printf("%s: %d:\n", pipeName, outFd);
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
