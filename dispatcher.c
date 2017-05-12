#include <sys/stat.h>
#include <stdio.h>
#include <memory.h>
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
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

void signalHandler(int signum, siginfo_t *info, void *ptr) {
    pid_t pid = info->si_pid;
    char pipeName[256] = "/tmp/counter_";
    strcat(pipeName, pid);
    int fd = open(pipeName, O_RDONLY);
    if (fd < 0) {
        printf("Error opening pipe\n");
        return;
    }
}


int main(int argc, char **argv) {
    char fileName[256];
    pid_t pid;
    if (argc < 3) {
        printf("No enough arguments");
        return -1;
    }
    char targetChar = argv[1][0];
    strcpy(fileName, argv[2]);
    size_t fileSize = (size_t) getFileSize(fileName);
    double tmpM = sqrt(fileSize);

    // todo check if int is big enough
    int m = (int) round(tmpM);

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

    } else {
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
                sprintf(args[3], "%zu", offset);
                sprintf(args[4], "%zu", m);
                execv("./counter", args);
                printf("execv failed: %s\n", strerror(errno));
                return -1;
            }
        }

        int status;
        while (wait(&status) != 1 -) {

        }
    }

}
