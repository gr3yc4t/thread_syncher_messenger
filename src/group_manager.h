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
    dev_t deviceID;             //TODO: remove as already present in "cdev"

    int group_id;               //Returned by the IDR

    int message_variable;

    struct device* dev;
    group_t *descriptor;
} group_data;


static int openGroup(struct inode *inode, struct file *file);
static ssize_t readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);
static ssize_t writeGroupMessage(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);




static struct file_operations group_operation = {
    .owner = THIS_MODULE,
    .open = openGroup,
    .read = readGroupMessage,
    .write = writeGroupMessage,
    //.release = my_release,
    //.unlocked_ioctl = my_ioctl
};


// Global Variables

static struct class *group_class;




int registerGroupDevice(group_data *grp_data, const struct device* parent);

void unregisterGroupDevice(group_data *grp_data);





#endif //GRP_MAN_H