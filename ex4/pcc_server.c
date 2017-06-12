#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>


void signalHandler(int signum, siginfo_t *info, void *ptr) {

}


int main(int argc, char **argv) {

    // CREDIT: HW3
    struct sigaction new_action;
    memset(&new_action, 0, sizeof(new_action));
    new_action.sa_sigaction = signalHandler;
    new_action.sa_flags = SA_SIGINFO;
    if (0 != sigaction(SIGINT, &new_action, NULL)) {
        printf(
                "Signal handle registration "
                        "failed. %s\n",
                strerror(errno));
        return -1;
    }

    int stat[95];
    int totalBytesRead = 0;
    pthread_mutex_t count_mutex;
}