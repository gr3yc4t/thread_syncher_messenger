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
} T_GROUPS_LST;


/** @brief Main device data */
typedef struct t_main_sync {
	int minor;								/**< minor# */

	char buffer[256];						//TODO: adjust

	T_GROUPS_LST groups_lst;
	struct idr group_map;

	struct semaphore sem;
} T_MAIN_SYNC;

/*------------------------------------------------------------------------------
	Prototype Declaration
------------------------------------------------------------------------------*/
int mainInit(void);
void mainExit(void);

void initializeMainDevice(void);
int installGroup(group_t *new_group);

static int mainOpen(struct inode *inode, struct file *filep);
static int mainRelease(struct inode *inode, struct file *filep);
static ssize_t mainWrite(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);
static ssize_t mainRead(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);

long int mainDeviceIoctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);


static int sRegisterMainDev(void);
static void sUnregisterMainDev(void);




/*------------------------------------------------------------------------------
	Global Variables
------------------------------------------------------------------------------*/
static struct class *g_class;				/**< device class */
static struct cdev *g_cdev_array;			/**< charactor devices */
static int g_dev_major = D_DEV_MAJOR;		/**< major# */
static int g_dev_minor = D_DEV_MINOR;		/**< minor# */
static int g_buf[D_DEV_NUM][D_BUF_SIZE];	/**< buffer (for sample-code) */

//TODO: change to a more meaningful name
static struct device *main_dev;				//Used for "parent" field in device_create; 

//Main device global pointer 
static T_MAIN_SYNC *main_device;		


static struct file_operations g_fops;


#endif //MAIN_DEV_H