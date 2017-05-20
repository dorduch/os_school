#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char **argv) {
  bool finished = false;
  if (argc != 5) {
    printf("Invalid args\n");
    return -1;
  }
  size_t cnt = 0;
  char targetChar = argv[1][0];
  char fileName[256];
  char pipeName[256] = "/tmp/counter_";
  strcpy(fileName, argv[2]);
  off_t offset;
  sscanf(argv[3], "%jd", &offset);
  size_t length;
  sscanf(argv[4], "%zu", &length);
  if (length == 0) {
    printf("empty file\n");
    exit(0);
  }
  int fd = open(fileName, O_RDONLY);
  if (fd < 0) {
    printf("Error opening file to read: %s\n", strerror(errno));
    return -1;
  }

  // CREDIT mmap: recitation 5
  char *arr = (char *)mmap(NULL, length, PROT_READ, MAP_SHARED, fd, offset);
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
  close(fd);

  pid_t myPid = getpid();
  sprintf(pipeName, "%s%d", pipeName, myPid);
  if (mkfifo(pipeName, 0666) == -1) {
    printf("Error when trying to mkfifo: %s\n", strerror(errno));
    return -1;
  }
  void signalHandler(int signum, siginfo_t *info, void *ptr) {
    pid_t pid = info->si_pid;
    if (pid != 0) {
      int outFd = open(pipeName, O_WRONLY);
      if (outFd < 0) {
        printf("Error opening pipe to write: %s\n", strerror(errno));
        return;
      }
      write(outFd, &cnt, sizeof(cnt));
      sleep(1);
      if (munmap(arr, length) == -1) {
        printf("Error un-mmapping the file: %s\n", strerror(errno));
        exit(-1);
      }
      close(outFd);
      unlink(pipeName);
      finished = true;
      exit(0);
    }
    return;
  }
  struct sigaction new_action;
  memset(&new_action, 0, sizeof(new_action));
  new_action.sa_sigaction = signalHandler;
  new_action.sa_flags = SA_SIGINFO;
  if (0 != sigaction(SIGUSR2, &new_action, NULL)) {
    printf(
        "Signal handle registration "
        "failed. %s\n",
        strerror(errno));
    return -1;
  }

  pid_t dispatcherPid = getppid();

  while (!finished) {
    if (kill(dispatcherPid, SIGUSR1) == -1) {
      printf(
          "Error while trying to send signal from counter to dispatcher, "
          "killing counter %d: %s\n",
          myPid, strerror(errno));
      unlink(pipeName);
      return -1;
    }
  }
  exit(0);
}
