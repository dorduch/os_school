#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h> 
#include <string.h>
#include <unistd.h>    
#include "message_slot.h"

int main(int argc, char **argv) {
  int file_desc, ret_val, channel_index;
  char message[BUF_LEN];
  if (argc != 2) {
    printf("invalid arguments\n");
    exit(-1);
  }

  sscanf(argv[1], "%d", &channel_index);

  // CREDIT - Recitation
  file_desc = open("/dev/"DEVICE_FILE_NAME, O_RDONLY);
  if (file_desc < 0) {
    printf("Can't open device file: %s, %s\n", DEVICE_FILE_NAME, strerror(errno));
    exit(-1);
  }

  ret_val = ioctl(file_desc, IOCTL_SET_ENC, channel_index);

  if (ret_val < 0) {
    printf("ioctl_set_msg failed:%d\n", ret_val);
    exit(-1);
  }

  
  if (read(file_desc, message, BUF_LEN) < 0) {
    printf("error while writing to file %s\n", strerror(errno));
  }
  printf("write success\n");  
  close(file_desc);
  printf("message %s was received from device %s on channel %d\n", message, DEVICE_FILE_NAME, channel_index);
  return 0;
}
