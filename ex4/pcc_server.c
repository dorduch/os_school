#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int stat[95];
int totalBytesRead = 0;
pthread_mutex_t count_mutex;
bool infiniteLoop = true;
int numOfThreads = 0;
int listenfd = -1;

void signalHandler(int signum, siginfo_t *info, void *ptr) {
  int i;
  infiniteLoop = false;
  if (close(listenfd) == -1) {
    printf("\n Error while closing fd %s \n", strerror(errno));
  };
  while (numOfThreads > 0) {
    sleep(1);
  }
  printf("total bytes: %d\n", totalBytesRead);
  for (i = 0; i < 95; i++) {
    printf("%c: %d\n", i + 32, stat[i]);
  }
  exit(0);
}

void *threadLogic(void *t) {
  int socketFd = (long)t;

  if (socketFd < 0) {
    printf("\n Error : Accept Failed. %s \n", strerror(errno));
    exit(-1);
  }
  __sync_fetch_and_add(&numOfThreads, 1);
  int localStat[95];
  size_t printable = 0;
  int i;
  for (i = 0; i < 95; i++) {
    localStat[i] = 0;
  }
  char buffer[1024];
  size_t len;
  if (read(socketFd, &len, sizeof(len)) == -1) {
    printf("\n Error : read from socket %s \n", strerror(errno));
    if (close(socketFd) == -1) {
      printf("\n Error while closing fd %s \n", strerror(errno));
    };
    exit(-1);
  }
  size_t bytesRead = 0;
  while (bytesRead < len) {
    size_t toRead =
        bytesRead + sizeof(buffer) < len ? sizeof(buffer) : len - bytesRead;
    ssize_t bytes = read(socketFd, buffer, toRead);
    if (bytes == -1) {
      printf("\n Error : can't read from socket %s \n", strerror(errno));
      if (close(socketFd) == -1) {
        printf("\n Error while closing fd %s \n", strerror(errno));
      };
      exit(-1);
    }
    bytesRead += bytes;
    for (i = 0; i < toRead; i++) {
      char current = buffer[i];
      if (current < 127 && current > 31) {
        localStat[current - 32]++;
        printable++;
      }
    }
  }
  if (write(socketFd, &printable, sizeof(printable)) == -1) {
    printf("\n Error : can't write to socket %s \n", strerror(errno));
    if (close(socketFd) == -1) {
      printf("\n Error while closing fd %s \n", strerror(errno));
    };
    exit(-1);
  };

  if (close(socketFd) == -1) {
    printf("\n Error while closing fd %s \n", strerror(errno));
  };

  pthread_mutex_lock(&count_mutex);
  for (i = 0; i < 95; i++) {
    stat[i] += localStat[i];
  }
  totalBytesRead += bytesRead;
  pthread_mutex_unlock(&count_mutex);
  __sync_fetch_and_add(&numOfThreads, -1);
  pthread_exit(NULL);
}

int main(int argc, char **argv) {
  int i;
  for (i = 0; i < 95; i++) {
    stat[i] = 0;
  }
  // CREDIT recitation
  int rc = pthread_mutex_init(&count_mutex, NULL);
  if (rc) {
    printf(
        "ERROR in pthread_mutex_init(): "
        "%s\n",
        strerror(rc));
    exit(-1);
  }

  // CREDIT: HW3
  struct sigaction new_action;
  memset(&new_action, 0, sizeof(new_action));
  new_action.sa_sigaction = signalHandler;
  new_action.sa_flags = SA_SIGINFO;
  if (0 != sigaction(SIGINT, &new_action, NULL)) {
    printf("Signal handle registration failed. %s\n", strerror(errno));
    exit(-1);
  }

  long connfd = -1;

  struct sockaddr_in serv_addr;
  // CREDIT recitation
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset(&serv_addr, '0', sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(2233);

  if (0 != bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
    printf("\n Error : Bind Failed. %s \n", strerror(errno));
    exit(-1);
  }

  if (0 != listen(listenfd, 10)) {
    printf("\n Error : Listen Failed. %s \n", strerror(errno));
    exit(-1);
  }

  while (infiniteLoop) {
    socklen_t addrsize = sizeof(struct sockaddr_in);
    connfd = accept(listenfd, NULL, NULL);

    if (connfd > 0) {
      pthread_t thread;
      rc = pthread_create(&thread, NULL, threadLogic,(void*)connfd);
      if (rc) {
        printf("ERROR in pthread_create(): %s\n", strerror(rc));
        exit(-1);
      }
    }
  }
}
