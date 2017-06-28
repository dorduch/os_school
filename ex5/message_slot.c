/* tmp = current_ = mu = r tmp_list_nodel_s =
    (message_slot_list_node *)kmalloc(sizeof(message_slot_list_node),
                                      GFP_KERNEL) t->ltmpe_stmp_list_node = mu =
        r tmp_list_nodel_st->ltmpe_s ltt & _list_nmpe_ * oiop_lise_noli;
t_usrent_ - &->>->> --->> -* >>->e->me->next->>->>> &->->>> od e->mes->->>
    slo1->channels[current_list_node->current_index]
#undef MODULE
#define MODULE /* Not a permanent part, though. */

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

typedef struct message_slot { char channels[BUF_LEN][4]; } message_slot;
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
static int major; /* device major number */

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file) {
  unsigned long flags;  // for spinlock
  message_slot_list_node* current_list_node;
  message_slot_list_node* next_list_node = (message_slot_list_node*)kmalloc(sizeof(message_slot_list_node), GFP_KERNEL);
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
           printk("in while new node\n");
    current_list_node = current_list_node->next;
  }
  if (current_list_node->id != file->f_inode->i_ino) {
    printk("creating message slot for file %d\n", file->f_inode->i_ino);
    // current_list_node.next = (message_slot_list_node *)kmalloc(
    //     sizeof(message_slot_list_node), GFP_KERNEL);
    // if (current_list_node.next == NULL) {
    //   printk("error when allocating message_slot_list_node\n");
    // }
    // next_list_node = *(current_list_node.next);
    next_list_node->id = file->f_inode->i_ino;
    next_list_node->next = NULL;
    next_list_node->message_slot1 =
        (message_slot *)kmalloc(sizeof(message_slot), GFP_KERNEL);
    next_list_node->current_index = -1;
    current_list_node->next = next_list_node;
    printk("created new node %p\n", current_list_node->next);

  } else {
    printk("message slot exists for file\n");
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
static ssize_t device_read(
    struct file *file,   /* see include/linux/fs.h   */
    char __user *buffer, /* buffer to be filled with data */
    size_t length,       /* length of the buffer     */
    loff_t *offset) {
  message_slot_list_node* current_list_node;
  int i;
  printk("in read to %d\n", file->f_inode->i_ino);
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
  printk("found the node, writing from channel %d\n",
         current_list_node->current_index);

  for (i = 0; i < length && i < BUF_LEN; i++) {
    put_user(current_list_node -> message_slot1 ->channels[current_list_node->current_index][i],
             buffer + i);
  }
  printk("wrote %d\n", i);
  return i;
}

static long device_ioctl(                      // struct inode*  inode,
    struct file *file, unsigned int ioctl_num, /* The number of the ioctl */
    unsigned long ioctl_param)                 /* The parameter to it */
{
 

  message_slot_list_node* current_list_node;
  printk("in ioctl to %d\n", file->f_inode->i_ino);

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
      printk("in while %d\n", current_list_node->id);
      current_list_node = current_list_node->next;
    }

    if (current_list_node->id != file->f_inode->i_ino) {
      printk("node id not found\n");
      return -1;
    }
    current_list_node->current_index = index;
    printk("index set to %d\n", index);

    /* Get the parameter given to ioctl by the process */
  }

  return SUCCESS;
}

/* somebody tries to write into our device file */
static ssize_t device_write(struct file *file, const char __user *buffer,
                            size_t length, loff_t *offset) {
  printk("in write to %d\n", file->f_inode->i_ino);

  int i;
  message_slot_list_node* current_list_node;

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
  printk("found the node, writing to channel %d\n",
         current_list_node->current_index);
  for (i = 0; i < length && i < BUF_LEN; i++) {
    get_user(current_list_node->message_slot1->channels[current_list_node->current_index][i],
             buffer + i);
  }
  printk("messaged received: %s\n", buffer);
  printk("messaged saved: %s\n", current_list_node->message_slot1->channels[current_list_node->current_index]);
  // for (i; i < BUF_LEN; i++) {
  //   get_user((*(current_list_node.message_slot1))
  //                .channels[current_list_node.current_index][i],
  //            '0');
  // }
  printk("wrote %d\n", i);

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
  major =
      register_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME, &Fops);  // todo check error

  /*
   * Negative values signify an error
   */
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

/* Cleanup - unregister the appropriate file from /proc */
static void __exit cleanup(void) {
  printk("in exit\n");
  struct message_slot_list_node* current_list_node = &first_node;
    struct message_slot_list_node* tmp_list_node;

  while (current_list_node->next != NULL) {
    printk("freeing message_slot\n");
    kfree(current_list_node->next->message_slot1);
    tmp_list_node = current_list_node->next;
    kfree(current_list_node->next);

    current_list_node = tmp_list_node;
  }

  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

module_init(init);
module_exit(cleanup);
