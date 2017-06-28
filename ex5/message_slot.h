#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>



#define MAJOR_NUM 245
#define IOCTL_SET_ENC _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "simple_message_slot"
#define SUCCESS 0


#endif
