/* Declare what kind of code we want from the header files
   Defining __KERNEL__ and MODULE allows us to access kernel-level
   code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE /* Not a permanent part, though. */

/* ***** Example w/ minimal error handling - for ease of reading ***** */

#include <asm/uaccess.h>  /* for get_user and put_user */
#include <linux/fs.h>     /* for register_chrdev */
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/module.h> /* Specifically, a module */
#include <linux/string.h> /* for memset. NOTE - not string.h!*/

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 80
#define DEVICE_FILE_NAME "message_slot"

struct chardev_info {
  spinlock_t lock;
};

struct message_slot {
  char **channels;
};

struct message_slot_list_node {
  message_slot *message_slot1;
  message_slot_list_node *next;
  //   message_slot_list_node *prev;
  char *id;
  int current_index;
};

static struct message_slot_list_node first_node;
static int dev_open_flag =
    0; /* used to prevent concurent access into the same device */
static struct chardev_info device_info;
static char Message[BUF_LEN]; /* The message the device will give when asked */
static int major;             /* device major number */

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file) {
  unsigned long flags;  // for spinlock
  printk("device_open(%p)\n", file);

  /*
   * We don't want to talk to two processes at the same time
   */
  spin_lock_irqsave(&device_info.lock, flags);
  if (dev_open_flag) {
    spin_unlock_irqrestore(&device_info.lock, flags);
    return -EBUSY;
  }

  dev_open_flag++;
  struct message_slot_list_node *current_list_node = first_node;
  while (current_list_node->next != NULL &&
         strcmp(current_list_node->id, file->f_inode->i_ino) != 0) {
    current_list_node = current_list_node->next;
  }
  if (strcmp(current_list_node->id, file->f_inode->i_ino) != 0)) {
      // current is good
    }
  else {
    current_list_node->next =
        (message_slot_list_node *)kmalloc(sizeof(message_slot_list_node));
    current_list_node = current_list_node->next;
    current_list_node->id = file->f_inode->i_ino;
    current_list_node->next = NULL;
    current_list_node->message_slot1 =
        (*message_slot)kmalloc(sizeof(message_slot));
    message_slot message_slot1 = current_list_node->message_slot1;
    int i;
    for (i = 0; i < 4; i++) {
      message_slot1[i] = (char *)kmalloc(128);
    }
  }

  spin_unlock_irqrestore(&device_info.lock, flags);

  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
  unsigned long flags;  // for spinlock
  printk("device_release(%p,%p)\n", inode, file);

  /* ready for our next caller */
  spin_lock_irqsave(&device_info.lock, flags);
  dev_open_flag--;
  spin_unlock_irqrestore(&device_info.lock, flags);

  return SUCCESS;
}

/* a process which has already opened
   the device file attempts to read from it */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
                           char __user

                               *buffer,   /* buffer to be filled with data */
                           size_t length, /* length of the buffer     */
                           loff_t *offset) {
  /* read doesnt really do anything (for now) */
  printk(
      "device_read(%p,%d) - operation not supported yet (last written - %s)\n",
      file, length, Message);

  return -EINVAL;  // invalid argument error
}

static long device_ioctl(                      // struct inode*  inode,
    struct file *file, unsigned int ioctl_num, /* The number of the ioctl */
    unsigned long ioctl_param)                 /* The parameter to it */
{

  /* Switch according to the ioctl called */
  if (IOCTL_SET_ENC == ioctl_num) {
      int index = (int)*ioctl_param;
      if (index < 0 || index > 3) {
            // todo bom
          return -1;
      }

    /* Get the parameter given to ioctl by the process */
    if (ioctl_param < 0 ||)
    printk("chardev, ioctl: setting encryption flag to %ld\n", ioctl_param);
    encryption_flag = ioctl_param;
  }

  return SUCCESS;
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char __user

                                                   *buffer,
                            size_t length, loff_t *offset) {
  int i;

  printk("device_write(%p,%d)\n", file, length);

  for (i = 0; i < length && i < BUF_LEN; i++) get_user(Message[i], buffer + i);

  /* return the number of input characters used */
  return i;
}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */
struct file_operations Fops = {
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .read = device_read,
    .write = device_write,     /* a.k.a. close */
    .release = device_release, /* a.k.a. close */
};

/* Called when module is loaded.
 * Initialize the module - Register the character device */
static int __init init(void) {
  /* init dev struct*/
  memset(&device_info, 0, sizeof(struct chardev_info));
  spin_lock_init(&device_info.lock);

  /* Register a character device. Get newly assigned major num */
  major = register_chrdev(0, DEVICE_RANGE_NAME,
                          &Fops /* our own file operations struct */);

  /*
   * Negative values signify an error
   */
  if (major < 0) {
    printk(KERN_ALERT "%s failed with %d\n",
           "Sorry, registering the character device ", major);
    return major;
  }
  first_node.next = NULL;
  //   first_node.prev = NULL;
  first_node.message_slot = NULL;
  printk("Registeration is a success. The major device number is %d.\n", major);
  printk("If you want to talk to the device driver,\n");
  printk("you have to create a device file:\n");
  printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, major);
  printk("You can echo/cat to/from the device file.\n");
  printk("Dont forget to rm the device file and rmmod when you're done\n");

  return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit cleanup(void) {
  /*
   * Unregister the device
   * should always succeed (didnt used to in older kernel versions)
   */

  // free them all

  unregister_chrdev(major, DEVICE_RANGE_NAME);
}

module_init(init);
module_exit(cleanup);
