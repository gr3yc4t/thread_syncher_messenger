#include "main_device.h"


/** file operations */
static struct file_operations main_fops = {
	.open    = mainOpen,
	.release = mainRelease,
	.write   = mainWrite,
	.read    = mainRead,
	.unlocked_ioctl = mainDeviceIoctl
};


int getGroupID(const group_t new_group);
int copy_group_t_from_user(__user group_t *user_group, group_t *kern_group);


/**
 * @brief Kernel Module Init
 *
 * @param nothing
 *
 * @retval 0		success
 * @retval CLASS_EXISTS if the class already exists
 * @retval others	failure
 */
int mainInit(void){
	int ret;

	pr_info("%s loading ...\n", D_DEV_NAME);

	//Try to install the 'group_device_class'
	if(installGroupClass() < 0)
		return CLASS_EXISTS;


	// Register devices 
	if ((ret = sRegisterMainDev()) != 0) {
		printk(KERN_ERR "register_dev() failed\n");
		return ret;
	}

	initializeMainDevice();

	return 0;
}

/**
 * @brief Kernel Module Exit
 * @todo Check that every structure is correctly deallocated
 * @param nothing
 * @retval nothing
 * 
 */
void mainExit(void){
	group_data *cursor;
	int id_cursor;

	printk(KERN_INFO "%s unloading ...\n", D_DEV_NAME);


	printk(KERN_INFO "Starting deallocating group devices...");

	idr_for_each_entry(&main_device_data.group_map, cursor, id_cursor){
		unregisterGroupDevice(cursor, false);	
		printk(KERN_INFO "Phase 1- Device %s destroyed", cursor->descriptor.group_name);
	}

	class_destroy(group_device_class);
	printk(KERN_INFO "Group class destroyed");

	idr_for_each_entry(&main_device_data.group_map, cursor, id_cursor){
		unregisterGroupDevice(cursor, true);	
		printk(KERN_INFO "Phase 2- Device %s destroyed", cursor->descriptor.group_name);
		
			#ifndef DISABLE_SYSFS
				if(cursor->flags.sysfs_loaded == 1){
					pr_debug("Releasing sysfs for group %d", id_cursor);
					releaseSysFs(&cursor->group_sysfs);
				}
			#endif
		kfree(cursor);	//Deallocate group_data structure
	}


	//Deallocate the IDR
	idr_destroy(&main_device_data.group_map);
	pr_debug("IDR destroyed");



	// Unregister the main devices
	sUnregisterMainDev();

	printk(KERN_INFO "Unloading completed");
}


/**
 * 	@brief Init Main Device members
 * 	@param nothing
 * 	@retval nothing
 * 
 * 	@note If a new structure is addedd to 't_main_sync' all init 
 *  	procedures should be perfomed here
 */
void initializeMainDevice(void){
	pr_debug("Initializing groups list...");

	idr_init(&main_device_data.group_map);	//Init group IDR
	sema_init(&main_device_data.sem, 1);	//Init main device semaphore
}






/**
 * @brief Kernel Module Open : open()
 *
 * @param [in]		inode	inode structure
 * @param [in,out]	filep	file structure
 *
 * @retval 0		success
 * @retval others	failure
 */
static int mainOpen(struct inode *inode, struct file *filep){
	printk(KERN_INFO "%s opening ...\n", D_DEV_NAME);

	//Store data into 'device_info' (kept per device)
	//filep->device_info = main_device_data;

	//Dev should only answare to ioctl request so no private data is needed
	

	return 0;
}

/**
 * @brief Kernel Module Release : close()
 *
 * @param [in]	inode	inode structure
 * @param [in]	filep	file structure
 *
 * @retval 0		success
 * @retval others	failure
 */
static int mainRelease(struct inode *inode, struct file *filep)
{
	//T_MAIN_SYNC *info = (T_MAIN_SYNC *)filep->device_info;

	printk(KERN_INFO "%s releasing ...\n", D_DEV_NAME);

	/* deallocate private data */
	//kfree(info);

	return 0;
}

/**
 * @brief Kernel Module Write : write()
 *
 * @param [in]		filep	file structure
 * @param [in]		buf		buffer address (user)
 * @param [in]		count	write data size
 * @param [in,out]	f_pos	file position
 *
 * @return	number of write byte
 */
static ssize_t mainWrite(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos){
	//Unimplemented
	return 0;
}

/**
 * @brief Kernel Module Read : read()
 *
 * @param [in]		filep	file structure
 * @param [out]		buf		buffer address (user)
 * @param [in]		count	read data size
 * @param [in,out]	f_pos	file position
 *
 * @bug Experimental feature
 * 
 * @return	number of read byte
 */
static ssize_t mainRead(struct file *filep, char __user *buf, size_t count, loff_t *f_pos){

	int group_id;
	group_data *grp_data;
	group_t descriptor;
	size_t len;

	group_id = (int) *f_pos;

	if(group_id < 0 ){
		printk(KERN_INFO "mainRead: conversion error, exiting...");
		return -1;
	}

	printk(KERN_INFO "mainRead: readed f_pos value: %ud", group_id);

	down(&main_device_data.sem);
		grp_data = idr_get_next(&main_device_data.group_map, &group_id);
	up(&main_device_data.sem);


	descriptor = grp_data->descriptor;

	len = descriptor.name_len;
	

	if( copy_to_user(buf, descriptor.group_name, len) > 0){
		printk(KERN_ERR "mainRead: Unable to copy memory to user");
	}

	return 0;
}

/*------------------------------------------------------------------------------
	Functions (Internal)
------------------------------------------------------------------------------*/
/**
 * @brief Register Main Devices
 *
 * @param nothing
 *
 * @retval 0 on success
 * @retval negative number on failure
 */
static int sRegisterMainDev(void){
	int r;
	int ret;

	/* acquire major#, minor# */
	if ((ret = alloc_chrdev_region(&main_device_data.dev, D_DEV_MINOR, D_DEV_NUM, D_DEV_NAME)) < 0) {
		printk(KERN_ERR "alloc_chrdev_region() failed while registering main_device\n");
		return ret;
	}

	main_dev_major = MAJOR(main_device_data.dev);
	main_dev_minor = MINOR(main_device_data.dev);

	/* create device class */
	main_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(main_class)) {
		if(PTR_ERR(main_class) == -EEXIST){
			printk(KERN_WARNING "Class 'main_sync' already exists");
		}else{
			printk(KERN_ERR "Canno create class 'main_sync', error %ld", PTR_ERR(main_class));
			goto cleanup_region;
		}
	}

	main_device = device_create(main_class, NULL, main_device_data.dev, NULL, D_DEV_NAME);
	if(IS_ERR(main_device)){
		goto cleanup_class;
	}

	/* initialize charactor devices */
	cdev_init(&main_device_data.cdev, &main_fops);
	main_device_data.cdev.owner = THIS_MODULE;

	/* register charactor devices */
	if (cdev_add(&main_device_data.cdev, main_device_data.dev, 1) < 0) {
		printk(KERN_ERR "cdev_add() failed: minor# = %d\n", main_dev_minor);	
		goto cleanup;
	}


	return 0;


	cleanup:
		device_destroy(main_class, main_device_data.dev);
	cleanup_class:
		class_destroy(main_class);
	cleanup_region:
		unregister_chrdev_region(main_device_data.dev, 1);

		return -1;
}

/**
 * @brief Unregister Main Devices
 *
 * @param nothing
 *
 * @return nothing
 */
static void sUnregisterMainDev(void){

	/* delete charactor devices */
	cdev_del(&main_device_data.cdev);
	/* destroy device node */
	device_destroy(main_class, main_device_data.dev);
	pr_debug("Main device destroyed");

	/* destroy device class */
	class_destroy(main_class);
	pr_debug("Main class destroryed");

	unregister_chrdev_region(main_device_data.dev, 1);
	pr_debug("Char device region deallocated");
}


/**
 * @brief Handler of ioctl's request made on main device
 * 
 * Available ioctl:
 *	-IOCTL_INSTALL_GROUP: used to install a group corresponding to the provided 'group_t' 
 * 			structure, returns GROUP_EXISTS if the group already exists
 *	-IOCTL_GET_GROUP_ID: returns the ID corresponding to the provided 'group_t' structure
 * 			or -1 if the group does not exists
 * 
 */
static long int mainDeviceIoctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){

	int ret;
	group_t group_tmp; 	//Alias for the second IOCTL

	switch (ioctl_num){
	case IOCTL_INSTALL_GROUP:

		if(copy_group_t_from_user((group_t*)ioctl_param, &group_tmp) < 0){
			pr_err("'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}

		if(getGroupID(group_tmp) >= 0){
			printk(KERN_WARNING "The group already exists!!!");
			return GROUP_EXISTS;
		}


		pr_debug("Installing group [%s]...", group_tmp.group_name);
		ret = installGroup(group_tmp);

		if(ret < 0){
			pr_err("Unable to install a group, exiting");
			return ret;
		}


		pr_info("Group [%s] installed correctly", group_tmp.group_name);

		break;
	

	case IOCTL_GET_GROUP_ID:

		if(copy_group_t_from_user((group_t*)ioctl_param, &group_tmp) < 0){
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}

		pr_debug("Group name: %s\nLen: %ld", group_tmp.group_name, group_tmp.name_len);

		ret = getGroupID(group_tmp);

		pr_debug("Fetched Group ID: %d", ret);

		break;


	default:
		pr_err("Invalid IOCTL command provided: \n\tioctl_num=%u\n\tparam: %lu", ioctl_num, ioctl_param);
		ret = INVALID_IOCTL_COMMAND;
		break;
	}

	return ret;
}

/**
 * @brief Install a group for the provided 'group_t' descriptor
 * 
 * @param[in]	new_group_descriptor	The group descriptor
 * 
 * @retval The installed group's ID
 * @retval ALLOC_ERR if some memory allocation fails
 * @retval IDR_ERR If the IDR fails to allocate the ID
 * 
 * @note For error codes meaning see 'main_device.h'
 */

__must_check int installGroup(const group_t new_group_descriptor){

	group_data *new_group;
	int group_id;
	int ret = 0;

	new_group = (group_data*)kmalloc(sizeof(group_data), GFP_KERNEL);

	if(!new_group)
		return ALLOC_ERR;


	memset(&new_group->flags, 0, sizeof(g_flags_t));	//Reset all flags

	new_group->descriptor = new_group_descriptor;
	new_group->owner = current_uid().val;
	init_rwsem(&new_group->owner_lock);


	pr_debug("Group descriptor: [%s]", new_group->descriptor.group_name);


	//Allocate ID
	pr_debug("Allocating IDR");
	down(&main_device_data.sem);
		new_group->group_id  = idr_alloc(&main_device_data.group_map, new_group, GRP_MIN_ID, GRP_MAX_ID, GFP_KERNEL);
	up(&main_device_data.sem);
	pr_debug("Allocated IDR number %d", new_group->group_id);

	if(new_group->group_id  < 0){
		pr_err("Unable to allocate ID for the new group");
		ret = IDR_ERR;
		goto cleanup;
	}

	pr_debug("Registering Group device...");
	ret = registerGroupDevice(new_group, main_device);

	if(ret != 0){
		printk(KERN_ERR "Error: %d", ret);
		goto cleanup;
	}


	#ifndef DISABLE_SYSFS
		if((ret = initSysFs(new_group)) < 0 ){
			printk(KERN_ERR "Unable to initialize the sysfs interface");
			goto cleanup;
		}

		new_group->flags.sysfs_loaded = 1;
	#endif

	new_group->flags.initialized = 1;

	return new_group->group_id;	//Return the new group ID


	cleanup:
		//Free memory
		kfree(new_group);
		down(&main_device_data.sem);
			idr_remove(&main_device_data.group_map, group_id);
		up(&main_device_data.sem);
		return ret;
}

/**
 * @brief Return the ID of an existing group from 'group_t' structure
 * 
 * @note This function respect thread-safety
 * 
 * @param[in] new_group 	Pointer to a 'group_t' strucuture to check
 * @retval the group ID if the group exists
 * @retval, -1 if the group does not exists
 * 
 */
__must_check int getGroupID(const group_t new_group){
	group_data *curr_group;
	int id_cursor;

	if(new_group.group_name == NULL)
		return -1;

	down(&main_device_data.sem);
		idr_for_each_entry(&main_device_data.group_map, curr_group, id_cursor){
			
			pr_debug("Comparing [%s] with [%s]", curr_group->descriptor.group_name, new_group.group_name);

			if(curr_group->descriptor.group_name == NULL)
				goto cleanup;

			if(strncmp(curr_group->descriptor.group_name, new_group.group_name, DEVICE_NAME_SIZE) == 0){

				pr_info("Group exists with ID: %d", id_cursor);

				up(&main_device_data.sem);
				return id_cursor;
			}
		}

	cleanup:
	up(&main_device_data.sem);
	return -1;
}


/**
 * @brief Get the 'group_t' descriptor from the group ID
 * 
 * @param[in] group_id The group ID
 * @param[out] group_dest A pointer to a 'group_t' strucuture
 * 
 * @retval 0 on success
 * @retval -1 on error
 * 
 * @todo: test if the semaphore is necessary (idr_find)
 */
__must_check int getGroupInfo(unsigned long group_id, group_t *group_dest){
	group_data *grp_data;
	
	down(&main_device_data.sem);
		grp_data = idr_find(&main_device_data.group_map, group_id);
	up(&main_device_data.sem);

	if(grp_data == NULL)
		return -1;

	*group_dest = grp_data->descriptor;

	return 0;

}





MODULE_AUTHOR("Alessandro Cingolani");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("Threads syncher and message exchanger");
MODULE_VERSION("1.1");