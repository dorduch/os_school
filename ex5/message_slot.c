#include "message_slot.h"
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

struct chardev_info {
  spinlock_t lock;
};

typedef struct message_slot { char channels[4][BUF_LEN]; } message_slot;
typedef struct message_slot_list_node message_slot_list_node;
struct message_slot_list_node {
  message_slot *message_slot1;
  message_slot_list_node *next;
  int id;
  int current_index;
};

static message_slot_list_node first_node;
static int dev_open_flag = 0;
static struct chardev_info device_info;
static int major;

/***************** char device functions *********************/

static int device_open(struct inode *inode, struct file *file) {
  unsigned long flags;
  message_slot_list_node *current_list_node;
  message_slot_list_node *next_list_node = (message_slot_list_node *)kmalloc(
      sizeof(message_slot_list_node), GFP_KERNEL);
  current_list_node = &first_node;
  printk("device_open(%p)\n", file);

  spin_lock_irqsave(&device_info.lock, flags);
  if (dev_open_flag) {
    spin_unlock_irqrestore(&device_info.lock, flags);
    return -EBUSY;
  }

  dev_open_flag++;
  while (current_list_node->next != NULL &&
         current_list_node->id != file->f_inode->i_ino) {
    current_list_node = current_list_node->next;
  }
  if (current_list_node->id != file->f_inode->i_ino) {
    next_list_node->id = file->f_inode->i_ino;
    next_list_node->next = NULL;
    next_list_node->message_slot1 =
        (message_slot *)kmalloc(sizeof(message_slot), GFP_KERNEL);
    next_list_node->current_index = -1;
    current_list_node->next = next_list_node;

  } else {
    printk("message slot exists for file\n");
  }
  spin_unlock_irqrestore(&device_info.lock, flags);

  return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
  unsigned long flags;
  printk("device_release(%p,%p)\n", inode, file);

  spin_lock_irqsave(&device_info.lock, flags);
  dev_open_flag--;
  spin_unlock_irqrestore(&device_info.lock, flags);

  return SUCCESS;
}

static ssize_t device_read(struct file *file, char __user *buffer,
                           size_t length, loff_t *offset) {
  message_slot_list_node *current_list_node;
  int i;
  current_list_node = &first_node;
  while (current_list_node->id != file->f_inode->i_ino &&
         current_list_node->next != NULL) {
    current_list_node = current_list_node->next;
  }
  if (current_list_node->id != file->f_inode->i_ino) {
    printk("node id not found\n");
    return -1;
  }
  if (current_list_node->current_index == -1) {
    printk("index not set\n");
    return -1;
  }

  for (i = 0; i < length && i < BUF_LEN; i++) {
    put_user(current_list_node->message_slot1
                 ->channels[current_list_node->current_index][i],
             buffer + i);
  }
  return i;
}

static long device_ioctl(struct file *file, unsigned int ioctl_num,
                         unsigned long ioctl_param) {
  message_slot_list_node *current_list_node;
  /* Switch according to the ioctl called */
  if (IOCTL_SET_ENC == ioctl_num) {
    int index = ioctl_param;
    if (index < 0 || index > 3) {
      printk("index is not in range [0,3]\n");
      return -1;
    }
    current_list_node = &first_node;
    while (current_list_node->id != file->f_inode->i_ino &&
           current_list_node->next != NULL) {
      current_list_node = current_list_node->next;
    }

    if (current_list_node->id != file->f_inode->i_ino) {
      printk("node id not found\n");
      return -1;
    }
    current_list_node->current_index = index;
  }

  return SUCCESS;
}

static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
  int i;
  message_slot_list_node *current_list_node;

  current_list_node = &first_node;
  while (current_list_node->id != file->f_inode->i_ino &&
         current_list_node->next != NULL) {
    current_list_node = current_list_node->next;
  }
  if (current_list_node->id != file->f_inode->i_ino) {
    printk("node id not found\n");
    return -1;
  }
  if (current_list_node->current_index == -1) {
    printk("index not set\n");
    return -1;
  }
  for (i = 0; i < length && i < BUF_LEN; i++) {
    get_user(current_list_node->message_slot1
                 ->channels[current_list_node->current_index][i],
             buffer + i);
  }
  return i;
}

/************** Module Declarations *****************/

struct file_operations Fops = {
    .open = device_open,
    .unlocked_ioctl = device_ioctl,
    .read = device_read,
    .write = device_write,
    .release = device_release,
};

static int __init init(void) {
  memset(&device_info, 0, sizeof(struct chardev_info));
  spin_lock_init(&device_info.lock);
  major =
      register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);  // todo check error
  if (major < 0) {
    printk(KERN_ALERT "%s failed with %d\n",
           "Sorry, registering the character device ", MAJOR_NUM);
    return major;
  }
  first_node.next = NULL;
  first_node.message_slot1 = NULL;
  first_node.id = -1;
  printk("Registeration is a success. The major device number is %d.\n",
         MAJOR_NUM);
  printk("If you want to talk to the device driver,\n");
  printk("you have to create a device file:\n");
  printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR_NUM);
  printk("You can echo/cat to/from the device file.\n");
  printk("Dont forget to rm the device file and rmmod when you're done\n");

  return 0;
}

static void __exit cleanup(void) {
  struct message_slot_list_node *current_list_node = &first_node;
  struct message_slot_list_node *tmp_list_node;
  while (current_list_node->next != NULL) {
    kfree(current_list_node->next->message_slot1);
    tmp_list_node = current_list_node->next;
    kfree(current_list_node->next);

    current_list_node = tmp_list_node;
  }

  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(init);
module_exit(cleanup);
