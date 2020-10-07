/**
 * @file main_device.h
 * @brief Handles all procedures releated to the main synch device file
 */

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
	Error Codes
------------------------------------------------------------------------------*/
#ifndef DISABLE_SYSFS
	#include "sysfs.h"
#endif

#define INVALID_IOCTL_COMMAND   -1
#define ALLOC_ERR 			-2		/**< Returned when a 'kmalloc' fails*/	 
#define USER_COPY_ERR		-3		/**< Returned when copy_to_user/copy_from_user fails*/
#define IDR_ERR				-4		/**< Returned when the IDR fails */
#define MEM_ACCESS_ERR		-5		/**< Returned when 'access_ok' fails */
#define CLASS_EXISTS		-10		/**< Returned when the class name already exists */
#define CLASS_ERR			-11
#define DEV_CREATION_ERR    -12    
#define CDEV_ALLOC_ERR		-13
#define GROUP_EXISTS		-14



/*------------------------------------------------------------------------------
	IOCTL
------------------------------------------------------------------------------*/

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)
#define IOCTL_GET_GROUP_ID  _IOW('X', 100, group_t*)


/*------------------------------------------------------------------------------
	Defined Macros
------------------------------------------------------------------------------*/
#define D_DEV_NAME		"main_thread_synch"			/**< Main device name */
#define D_DEV_MAJOR		(0)							/**< Main device major# */
#define D_DEV_MINOR		(0)							/**< Main device minor# */
#define D_DEV_NUM		(1)							/**< Number of main device */


#define CLASS_NAME		"thread_synch"				/**< Main device class name */


#define GRP_MIN_ID 		0							/**< Group's min ID */
#define GRP_MAX_ID		255							/**< Group's max ID */

/*------------------------------------------------------------------------------
	Type Definition
------------------------------------------------------------------------------*/


/** @brief Main device structure 
 * 
 * 	This structure is associated with the 'main_thread_synch' device.
 *  Apart from containing the device specification, the field 'group_map'
 * 	employs Linux IDR in order to map group's 'group_data' structure to IDs
 * 
*/
typedef struct t_main_sync {
	dev_t dev;
	struct cdev cdev;						/**< Main Charcter device */
	int minor;								/**< minor# */

	struct idr group_map;					/**< IDR that maps group's ID to their structure*/

	struct semaphore sem;					/**< Main device IDR semaphore */ 

} main_sync_t;

/*------------------------------------------------------------------------------
	Prototype Declaration
------------------------------------------------------------------------------*/
int mainInit(void);
void mainExit(void);

void initializeMainDevice(void);
int installGroup(const group_t new_group);

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


static struct device *main_device;				//Used for "parent" field in device_create; 

//Main device global pointer 
static main_sync_t main_device_data;	//TODO use an array to manage multiple main device	

static struct file_operations main_fops;

extern struct class *group_device_class;



#endif //MAIN_DEV_H