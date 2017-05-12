#include <unistd.h>
#include <errno.h>
#include <memory.h>
#include <printf.h>
#include <sys/param.h>
#include <signal.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

//CREDIT hw2
off_t getFileSize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    fprintf(stderr, "Cannot determine size of %s: %s\n",
            filename, strerror(errno));

    return -1;
}

size_t result = 0;

void signalHandler(int signum, siginfo_t *info, void *ptr) {
    printf("signal handler!");
    pid_t pid = info->si_pid;
    char buffer[1024];
    char *pipeName = "/tmp/counter_";
    pipeName += sprintf(pipeName, "%d", pid);
//    strcat(pipeName, pid);
    int fd = open(pipeName, O_RDONLY);
    if (fd < 0) {
        printf("Error opening pipe: %s\n", strerror(errno));
        return;
    }
    ssize_t readBytes = read(fd, buffer, 1024);
    if (readBytes < 0) {
        printf("cannot read from pipe: %s\n", strerror(errno));
        return;
    }
    size_t toAdd = (size_t) buffer;
    result += toAdd;

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
    if (m < 1024) {
        pid = fork();
        printf("%d", pid);
        if (pid < 0) {
            printf("fork failed: %s\n", strerror(errno));
            return -1;
        }
        if (pid == 0) {
            printf("in fork!!!!");
            char *args[6];
            args[0] = "./counter";
            args[1] = argv[1];
            args[2] = argv[2];
            sprintf(args[3], "%lld", offset);
            sprintf(args[4], "%zu", fileSize);
            args[5] = NULL;
            execv("./counter", args);
            printf("execv failed: %s\n", strerror(errno));
            return -1;
        }
    }
    else {
        for (int i = 0; i < m; i++) {
            pid = fork();
            if (pid < 0) {
                printf("fork failed: %s\n", strerror(errno));
                return -1;
            }
            if (pid == 0) {
                char *args[5];
                args[0] = "./counter";
                args[1] = argv[1];
                args[2] = argv[2];
                sprintf(args[3], "%lld", offset);
                sprintf(args[4], "%zu", m);
                offset += m;
                execv("./counter", args);
                printf("execv failed: %s\n", strerror(errno));
                return -1;
            }
        }
        if (offset < fileSize) {
            size_t delta = fileSize - offset;
            pid = fork();
            if (pid < 0) {
                printf("fork failed: %s\n", strerror(errno));
                return -1;
            }
            if (pid == 0) {
                char *args[5];
                args[0] = "./counter";
                args[1] = argv[1];
                args[2] = argv[2];
                sprintf(args[3], "%lld", offset);
                sprintf(args[4], "%zu", delta);
                execv("./counter", args);
                printf("execv failed: %s\n", strerror(errno));
                return -1;
            }
        }
    }
    int status;
    while (wait(&status) != -1);
    printf("result: %zu\n", result);
    return 0;

}
