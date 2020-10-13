/**
 * @file		group_manager.h
 * @brief		Handles all procedures releated to file operation issued on a group device
 *
 */

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
#include <linux/cred.h>    //For current_uid()


#include "message.h"

/*------------------------------------------------------------------------------
	Error Codes
------------------------------------------------------------------------------*/
#define INVALID_IOCTL_COMMAND   -1
#define ALLOC_ERR 		        -2
#define MEM_ACCESS_ERR          -5
#define CLASS_EXISTS		    -10
#define CLASS_ERR		        -11
#define DEV_CREATION_ERR        -12
#define CDEV_ALLOC_ERR		    -13
#define EMPTY_LIST              -20
#define NODE_NOT_FOUND          -21
#define CHDEV_ALLOC_ERR         -22




#define GROUP_MAX_MINORS    255
#define DEVICE_NAME_SIZE    64

#define DEFAULT_MSG_SIZE 256
#define DEFAULT_STORAGE_SIZE 1024

//IOCTLS

#define IOCTL_GET_GROUP_DESC _IOR('Q', 1, group_t*)
#define IOCTL_SET_STRICT_MODE _IOW('Q', 101, bool)
#define IOCTL_CHANGE_OWNER _IOW('Q', 102, uid_t)


#ifndef DISABLE_DELAYED_MSG

    #define IOCTL_SET_SEND_DELAY _IOW('Y', 0, long)
    #define IOCTL_REVOKE_DELAYED_MESSAGES _IO('Y', 1)
    #define IOCTL_CANCEL_DELAY _IO('Y', 2)

#endif

#ifndef DISABLE_THREAD_BARRIER

    #define IOCTL_SLEEP_ON_BARRIER _IO('Z', 0)
    #define IOCTL_AWAKE_BARRIER _IO('Z', 1)

#endif





#define GROUP_CLASS_NAME "group_synch"



static int openGroup(struct inode *inode, struct file *file);
static int releaseGroup(struct inode *inode, struct file *file);
static ssize_t readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static ssize_t writeGroupMessage(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);
static long int groupIoctl(struct file *filep, unsigned int ioctl_num, unsigned long ioctl_param);
static int flushGroupMessage(struct file *filep, fl_owner_t id);


inline void initParticipants(group_data *grp_data);
int installGroupClass(void);

static struct file_operations group_operation = {
    .owner = THIS_MODULE,
    .open = openGroup,
    .read = readGroupMessage,
    .write = writeGroupMessage,
    .release = releaseGroup,
    .flush = flushGroupMessage,
    .unlocked_ioctl = groupIoctl
};


extern struct class *group_device_class;

int registerGroupDevice(group_data *grp_data, struct device* parent);
void unregisterGroupDevice(group_data *grp_data, bool flag);
int copy_group_t_from_user(__user group_t *user_group, group_t *kern_group);

#endif //GRP_MAN_H