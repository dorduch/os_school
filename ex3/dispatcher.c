#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// CREDIT hw2
off_t getFileSize(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) return st.st_size;
    fprintf(stderr, "Cannot determine size of %s: %s\n", filename,
            strerror(errno));
    return -1;
}

size_t res = 0;
int numOfForks = 1;
pid_t pidArr[16];
int pidIndex = 0;

pid_t forkLogic(char *targetChar, char *fileName, off_t offset, size_t length) {
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


void signalHandler(int signum, siginfo_t *info, void *ptr) {
    // CREDIT: register signal handler - Appendix A
    pid_t pid = info->si_pid;
    size_t toAdd;
    char pipeName[256] = "/tmp/counter_";
    sprintf(pipeName, "%s%d", pipeName, pid);
    if (pid != 0) {
        int k;
        for (k = 0; k < pidIndex; k++) {
            if (pidArr[k] == pid) {
                return;
            }
        }
        pidArr[pidIndex] = pid;
        pidIndex++;
        kill(pid, SIGUSR2);
        int fd = open(pipeName, O_RDONLY);
        if (fd < 0) {
            printf("Error opening pipe: %s\n", strerror(errno));
            return;
        }
        ssize_t readBytes = read(fd, &toAdd, sizeof(size_t));
        if (readBytes < 0) {
            printf("cannot read from pipe: %s\n", strerror(errno));
            close(fd);
            return;
        }
        close(fd);
        res += toAdd;
    }
    return;
}

size_t getPageSizeOffset(size_t fileSize) {
    size_t pageSize = (size_t) getpagesize();
    size_t res = pageSize;
    if (fileSize < (2 * pageSize)) {
        while (res < fileSize) {
            res += pageSize;
        }
        return res;
    }
    while (fileSize / res > 16) {
        res += pageSize;
    }

    return res;
};

int main(int argc, char **argv) {
    char fileName[256];
    if (argc < 3) {
        printf("No enough arguments\n");
        return -1;
    }
    strcpy(fileName, argv[2]);
    off_t fileSizeOffset = getFileSize(fileName);
    if (fileSizeOffset == -1) {
        return -1;
    }
    if (fileSizeOffset == 0) {
        printf("File is empty\n");
        return 0;
    }
    size_t fileSize = (size_t) fileSizeOffset;
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = signalHandler;
    new_action.sa_flags = SA_SIGINFO;
    if (0 != sigaction(SIGUSR1, &new_action, NULL)) {
        printf(
                "Signal handle registration "
                        "failed. %s\n",
                strerror(errno));
        return -1;
    }
    off_t offset = 0;
    size_t chunkSize = getPageSizeOffset(fileSize);
    while (offset < fileSize) {
        forkLogic(argv[1], argv[2], offset, chunkSize);
        numOfForks++;
        offset += chunkSize;
    }
    if (offset < fileSize) {
        size_t delta = fileSize - offset;
        forkLogic(argv[1], argv[2], offset, delta);
    }
    int j;
    for (j = 0; j < numOfForks; j++) {
        wait(NULL);
    }
    printf("result: %zu\n", res);
    return 0;
}
