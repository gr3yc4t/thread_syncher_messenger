#ifndef MAIN_DEV_H
#define MAIN_DEV_H


#include <linux/module.h>	/* MODULE_*, module_* */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/fs.h>		/* file_operations, alloc_chrdev_region, unregister_chrdev_region */
#include <linux/cdev.h>		/* cdev, dev_init(), cdev_add(), cdev_del() */
#include <linux/device.h>	/* class_create(), class_destroy(), device_create(), device_destroy() */
#include <linux/slab.h>		/* kmalloc(), kfree() */
#include <linux/uaccess.h>	/* copy_from_user(), copy_to_user() */
#include <linux/semaphore.h>	/* used acces to semaphore, process management syncronization behaviour */

#include <linux/ioctl.h>	
#include <linux/idr.h>



#include "group_manager.h"



/*------------------------------------------------------------------------------
	IOCTL
------------------------------------------------------------------------------*/

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)



/*------------------------------------------------------------------------------
	Defined Macros
------------------------------------------------------------------------------*/
#define D_DEV_NAME		"main_sync"			/**< device name */
#define D_DEV_MAJOR		(0)					/**< major# */
#define D_DEV_MINOR		(0)					/**< minor# */
#define D_DEV_NUM		(1)					/**< number of device */
#define D_BUF_SIZE		(32)				/**< buffer size (for sample-code) */

#define GRP_MIN_ID 		0
#define GRP_MAX_ID		255

/*------------------------------------------------------------------------------
	Type Definition
------------------------------------------------------------------------------*/

typedef struct t_groups {
	group_t group;
	struct list_head list;
} group_list_t;


/** @brief Main device data */
typedef struct t_main_sync {
	int minor;								/**< minor# */

	struct list_head groups_lst;
	struct idr group_map;

	struct semaphore sem;
} main_sync_t;

/*------------------------------------------------------------------------------
	Prototype Declaration
------------------------------------------------------------------------------*/
int mainInit(void);
void mainExit(void);

void initializeMainDevice(void);
int installGroup(const group_t *new_group);

static int mainOpen(struct inode *inode, struct file *filep);
static int mainRelease(struct inode *inode, struct file *filep);
static ssize_t mainWrite(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t mainRead(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);

static long int mainDeviceIoctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);


static int sRegisterMainDev(void);
static void sUnregisterMainDev(void);




/*------------------------------------------------------------------------------
	Global Variables
------------------------------------------------------------------------------*/
static struct class *main_class;				/**< device class */
static struct cdev *main_cdev;			/**< charactor devices */
static int main_dev_major = D_DEV_MAJOR;		/**< major# */
static int main_dev_minor = D_DEV_MINOR;		/**< minor# */

//TODO: change to a more meaningful name
static struct device *main_dev;				//Used for "parent" field in device_create; 

//Main device global pointer 
static main_sync_t *main_device;		

static struct file_operations main_fops;


#endif //MAIN_DEV_H