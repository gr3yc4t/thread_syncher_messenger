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

#define GROUP_MAX_MINORS    255
#define DEVICE_NAME_SIZE    64


/**
 * @brief system-wide descriptor of a group
 */
typedef struct group_t {
	unsigned int group_id;		//Thread group ID
	char *group_name;
} group_t;



typedef struct group_data {
    struct cdev cdev;           //Characted Device definition  
    dev_t deviceID;

    int message_variable;

    group_t *descriptor;
} group_data;


static int openGroup(struct inode *inode, struct file *file);
static int readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);





static struct file_operations group_operation = {
    .owner = THIS_MODULE,
    .open = openGroup,
    //.read = readGroupMessage,
    //.write = my_write,
    //.release = my_release,
    //.unlocked_ioctl = my_ioctl
};





int registerGroupDevice(unsigned int group_device, group_data *grp_data);






#endif //GRP_MAN_H