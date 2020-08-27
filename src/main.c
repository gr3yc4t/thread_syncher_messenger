/**
 * @file		main.c
 * @brief		Linux Kernel Module for Synchronization
 *
 */



#include <linux/module.h>	/* MODULE_*, module_* */
#include <linux/fs.h>		/* file_operations, alloc_chrdev_region, unregister_chrdev_region */
#include <linux/cdev.h>		/* cdev, dev_init(), cdev_add(), cdev_del() */
#include <linux/device.h>	/* class_create(), class_destroy(), device_create(), device_destroy() */
#include <linux/slab.h>		/* kmalloc(), kfree() */
#include <linux/uaccess.h>	/* copy_from_user(), copy_to_user() */


#include "main_device.h"


static int mainStart(void);
static void mainStop(void);


/*------------------------------------------------------------------------------
	Macro Calls
------------------------------------------------------------------------------*/
MODULE_AUTHOR("Alessandro Cingolani");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("Main module");
MODULE_VERSION("0.1");


module_init(mainStart);
module_exit(mainStop);






static int mainStart(void){

	printk(KERN_INFO "Loading AOSV Project module...");
	
	mainInit();

	return 0;
}



static void mainStop(void){
	printk(KERN_INFO "Unloading AOSV Project module...");

	mainExit();
}

