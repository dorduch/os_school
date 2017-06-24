#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h> 
#include <unistd.h>    

int main(int argc, char **argv) {
  int file_desc, ret_val, channel_index;
  char[BUF_LEN + 1] message;
  if (argc != 3) {
    printf("invalid arguments\n");
    exit(-1);
  }

  sscanf(argv[1], "%d", &channel_index);

  // CREDIT - Recitation
  file_desc = open("/dev/" DEVICE_FILE_NAME, 0);
  if (file_desc < 0) {
    printf("Can't open device file: %s, %s\n", DEVICE_FILE_NAME, strerror(errono));
    exit(-1);
  }

  ret_val = ioctl(file_desc, IOCTL_SET_ENC, channel_index);

  if (ret_val < 0) {
    printf("ioctl_set_msg failed:%d\n", ret_val);
    exit(-1);
  }
  // CREDIT - copy the first n chars :
  // https://stackoverflow.com/questions/16348512/getting-the-first-10-characters-of-a-string
  // https://linux.die.net/man/3/strncpy
  strncpy(message, argv[2], BUF_LEN + 1);
  message[BUF_LEN] = '\0';
  if (write(file_desc, message, sizeof(message)) < 0) {
    printf("error while writing to file %s\n", strerror(errono));
  }
  close(file_desc);
  printf("message %s was sent to device %s on channel %d\n", message, DEVICE_FILE_NAME, channel_index);
  return 0;
}
