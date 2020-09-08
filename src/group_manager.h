#ifndef GRP_MAN_H
#define GRP_MAN_H


#include <linux/module.h>	/* MODULE_*, module_* */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/fs.h>		/* file_operations, alloc_chrdev_region, unregister_chrdev_region */
#include <linux/cdev.h>		/* cdev, dev_init(), cdev_add(), cdev_del() */
#include <linux/device.h>	/* class_create(), class_destroy(), device_create(), device_destroy() */
#include <linux/slab.h>		/* kmalloc(), kfree() */
#include <linux/uaccess.h>	/* copy_from_user(), copy_to_user() */
#include <linux/semaphore.h>	/* used acces to semaphore, process management syncronization behaviour */

#include <linux/ioctl.h>
#include <linux/string.h>       //For snprintf

#include <linux/idr.h>
#include <linux/errno.h>


#include "message.h"



#define EMPTY_LIST -20
#define NODE_NOT_FOUND -21


#define GROUP_MAX_MINORS    255
#define DEVICE_NAME_SIZE    64

#define DEFAULT_MSG_SIZE 256
#define DEFAULT_STORAGE_SIZE 256



#ifndef DISABLE_DELAYED_MSG

    //IOCTLS
    #define IOCTL_SET_SEND_DELAY _IOW('Y', 0, long)
    #define IOCTL_REVOKE_DELAYED_MESSAGES _IO('Y', 1)

#endif

#ifndef DISABLE_THREAD_BARRIER

    #define IOCTL_SLEEP_ON_BARRIER _IO('Z', 0)
    #define IOCTL_AWAKE_BARRIER _IO('Z', 1)

#endif




static int openGroup(struct inode *inode, struct file *file);
static int releaseGroup(struct inode *inode, struct file *file);
static ssize_t readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static ssize_t writeGroupMessage(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);
static long int groupIoctl(struct file *filep, unsigned int ioctl_num, unsigned long ioctl_param);
static int flushGroupMessage(struct file *filep, fl_owner_t id);

inline void initParticipants(group_data *grp_data);


static struct file_operations group_operation = {
    .owner = THIS_MODULE,
    .open = openGroup,
    .read = readGroupMessage,
    .write = writeGroupMessage,
    .release = releaseGroup,
    .flush = flushGroupMessage,
    .unlocked_ioctl = groupIoctl
};


// Global Variables

static struct class *group_class;




int registerGroupDevice(group_data *grp_data, const struct device* parent);
void unregisterGroupDevice(group_data *grp_data);





#endif //GRP_MAN_H