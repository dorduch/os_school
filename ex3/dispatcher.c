#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>
#include <fcntl.h>

//CREDIT hw2
off_t getFileSize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return -1;
}

size_t res = 0;

void signalHandler(int signum, siginfo_t *info, void *ptr) {
    pid_t pid = info->si_pid;
    size_t toAdd;
    char pipeName[256] = "/tmp/counter_";
    sprintf(pipeName, "%s%d", pipeName, pid);
    int fd = open(pipeName, O_RDONLY);
    if (fd < 0) {
        printf("Error opening pipe: %s\n", strerror(errno));
        return;
    }
    ssize_t readBytes = read(fd, &toAdd, sizeof(size_t));
    if (readBytes < 0) {
        printf("cannot read from pipe: %s\n", strerror(errno));
        return;
    }
    res += toAdd;

}

pid_t forkLogic(char* targetChar, char* fileName, off_t offset, size_t length) {
    pid_t pid = fork();
    if (pid < 0) {
        printf("fork failed: %s\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        char *args[6];
        char offsetStr[1024];
        char sizeStr[1024];
        args[0] = "./counter";
        args[1] = targetChar;
        args[2] = fileName;
        sprintf(offsetStr, "%jd", offset);
        args[3] = offsetStr;
        sprintf(sizeStr, "%zu", length);
        args[4] = sizeStr;
        args[5] = NULL;
        execv("./counter", args);
        printf("execv failed: %s\n", strerror(errno));
        return -1;
    }
    return pid;
}


int main(int argc, char **argv) {

    char fileName[256];
    pid_t pid;
    if (argc < 3) {
        printf("No enough arguments");
        return -1;
    }
    strcpy(fileName, argv[2]);
    size_t fileSize = (size_t) getFileSize(fileName);
    double tmpM = sqrt(fileSize);
    size_t m = (size_t) floor(tmpM);

    // CREDIT: register signal handler - Appendix A
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = signalHandler;
    new_action.sa_flags = SA_SIGINFO;
    if (0 != sigaction(SIGUSR1, &new_action, NULL)) {
        printf("Signal handle registration "
                       "failed. %s\n", strerror(errno));
        return -1;
    }
    off_t offset = 0;
    if (m < 0) {
        forkLogic(argv[1], argv[2], offset, fileSize);
    }
    else {
        int i;
        m = 2;
        size_t chunkSize = (size_t) floor(fileSize/m);
        for (i = 0; i < m; i++) {
            printf("%d\n", i);
            forkLogic(argv[1], argv[2], offset, chunkSize);
            offset += chunkSize;
        }
        if (offset < fileSize) {
            size_t delta = fileSize - offset;
            forkLogic(argv[1], argv[2], offset, delta);
        }
    }
    int status;
    while (wait(&status) != -1);
    printf("result: %zu\n", res);
    return 0;

}
