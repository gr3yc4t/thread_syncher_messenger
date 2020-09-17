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
 * @retval others	failure
 */
int mainInit(void){
	int ret;

	printk(KERN_INFO "%s loading ...\n", D_DEV_NAME);

	//Try to install the 'group_class'
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
 * @bug Classes are not deallocated correctly so a reboot is necessary to reaload the module
 */
void mainExit(void){
	group_data *cursor;
	int id_cursor;

	printk(KERN_INFO "%s unloading ...\n", D_DEV_NAME);


	printk(KERN_INFO "Starting deallocating group devices...");

	idr_for_each_entry(&main_device_data.group_map, cursor, id_cursor){
		unregisterGroupDevice(cursor, false);	//TODO:Test if this is enough
		printk(KERN_INFO "1- Device %s destroyed", cursor->descriptor.group_name);
		
		//kfree(cursor);	//Deallocate group_data structure
	}


	class_destroy(group_class);


	idr_for_each_entry(&main_device_data.group_map, cursor, id_cursor){
		unregisterGroupDevice(cursor, true);	//TODO:Test if this is enough
		printk(KERN_INFO "2- Device %s destroyed", cursor->descriptor.group_name);
		
			#ifndef DISABLE_SYSFS
				printk(KERN_DEBUG "Releasing sysfs for group %d", id_cursor2);
				releaseSysFs(&cursor2->group_sysfs);
			#endif
		kfree(cursor);	//Deallocate group_data structure
	}

	printk(KERN_INFO "Group class destroyed");

	//Deallocate the IDR
	idr_destroy(&main_device_data.group_map);
	printk(KERN_DEBUG "IDR destroyed");



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
 * 	@todo Handle errors on initialization
 */
void initializeMainDevice(void){
	printk(KERN_INFO "Initializing groups list...");

	INIT_LIST_HEAD(&main_device_data.groups_lst);

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
	//Possible improvement: store into 'private_data' the creator of a group
	

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
 * @return	number of read byte
 */
static ssize_t mainRead(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{
	//Unimplemented
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
 * @retval 0		success
 * @retval others	failure
 */
static int sRegisterMainDev(void){
	int ret, i;

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
			printk(KERN_ERR "Canno create class 'main_sync', error %d", PTR_ERR(main_class));
			goto cleanup_region;
		}
	}

	main_device = device_create(main_class, NULL, main_device_data.dev, NULL, D_DEV_NAME "%u", main_dev_minor);
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
	printk(KERN_DEBUG "Main device destroryed");

	/* destroy device class */
	class_destroy(main_class);
	printk(KERN_DEBUG "Main class destroryed");

	unregister_chrdev_region(main_device_data.dev, 1);
	printk(KERN_DEBUG "Char device region deallocated");
}


/**
 * @brief Handler of ioctl's request made on main device
 * 
 * Available ioctl:
 * 	-IOCTL_INSTALL_GROUP: used to install a group corresponding to the provided 'group_t' 
 * 			structure, returns GROUP_EXISTS if the group already exists
 * 	-IOCTL_GET_GROUP_ID: returns the ID corresponding to the provided 'group_t' structure
 * 			or -1 if the group does not exists
 * 
 */
static long int mainDeviceIoctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){

	int ret;
	group_t group_tmp; 	//Alias for the second IOCTL
	char *group_name_tmp;

	switch (ioctl_num){
	case IOCTL_INSTALL_GROUP:

		printk(KERN_INFO "INSTALL GROUP ioctl issued");

		if(copy_group_t_from_user((group_t*)ioctl_param, &group_tmp)){
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}

		if(getGroupID(group_tmp) >= 0){
			printk(KERN_WARNING "The group already exists!!!");
			return GROUP_EXISTS;
		}


		printk(KERN_INFO "Installing group...");
		ret = installGroup(group_tmp);

		if(ret < 0){
			printk(KERN_INFO "Unable to install a group, exiting");
			return ret;
		}


		printk(KERN_INFO "Group installed correctly");

		break;
	

	case IOCTL_GET_GROUP_ID:

		if(copy_group_t_from_user((group_t*)ioctl_param, &group_tmp)){
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}

		ret = getGroupID(group_tmp);

		if(ret != -1)
			printk(KERN_DEBUG "Group found, returning the ID");

		break;


	default:
		printk(KERN_INFO "Invalid IOCTL command provided: \n\tioctl_num=%u\n\tparam: %lu", ioctl_num, ioctl_param);
		ret = -1;
		break;
	}

	return ret;
}

/**
 * @brief Install a group for the provided 'group_t' descriptor
 * 
 * @param[in]	new_group_descriptor	The group descriptor
 * 
 * @return 0 on success, negative number on error
 * 
 * @note For error codes meaning see 'main_device.h'
 */

int installGroup(const group_t new_group_descriptor){

	group_data *new_group;
	int group_id;
	int ret = 0;

	new_group = (group_data*)kmalloc(sizeof(group_data), GFP_KERNEL);

	if(!new_group)
		return ALLOC_ERR;

	new_group->descriptor = new_group_descriptor;
	new_group->owner = current->pid;

	down(&main_device_data.sem);

		//Allocate ID
		printk(KERN_DEBUG "Allocating IDR");
		new_group->group_id  = idr_alloc(&main_device_data.group_map, new_group, GRP_MIN_ID, GRP_MAX_ID, GFP_KERNEL);
		printk(KERN_DEBUG "Allocated IDR number %d", new_group->group_id);

	up(&main_device_data.sem);

	if(new_group->group_id  < 0){
		printk(KERN_ERR "Unable to allocate ID for the new group");
		ret = IDR_ERR;
		goto cleanup;
	}

	printk(KERN_DEBUG "Registering Group device...");
	int err = registerGroupDevice(new_group, main_device);

	if(err != 0){
		printk(KERN_ERR "Error: %d", err);
		ret = err;
		goto cleanup;
	}


	#ifndef DISABLE_SYSFS
		new_group->group_sysfs.manager = new_group->msg_manager;
		initSysFs(new_group);
	#endif
	return ret;


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
 * @return the group ID if the group exists, -1 otherwise
 * 
 */
int getGroupID(const group_t new_group){

	group_data *curr_group;
	int id_cursor;

	down(&main_device_data.sem);
		idr_for_each_entry(&main_device_data.group_map, curr_group, id_cursor){

			if(strncmp(curr_group->descriptor.group_name, new_group.group_name, DEVICE_NAME_SIZE) == 0){

				printk(KERN_INFO "Group exists with ID: %d", id_cursor);

				up(&main_device_data.sem);
				return id_cursor;
			}
		}
	up(&main_device_data.sem);
	return -1;
}


/**
 * @brief Copy a 'group_t' structure to kernel space
 * 
 * @param[in] user_group Pointer to a userspace 'group_t' structure
 * @param[out] kern_group Destination that will contain the 'group_t' structure
 * 
 * @return 0 on success, negative number on error
 * 
 */
int copy_group_t_from_user(__user group_t *user_group, group_t *kern_group){
		int ret;
		char *group_name_tmp;

		//Copy parameter from user space
		if(ret = copy_from_user(kern_group, user_group, sizeof(group_t))){	//Fetch the group_t structure from userspace
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}


		group_name_tmp = (char*) kmalloc(sizeof(char)*(kern_group->name_len), GFP_KERNEL);

		if(!group_name_tmp)
			return ALLOC_ERR;

		if(ret = copy_from_user(group_name_tmp, kern_group->group_name, sizeof(char)*kern_group->name_len)){	//Fetch the group_t structure from userspace
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}
		//Switch pointers
		kern_group->group_name = group_name_tmp;

		return 0;
}




MODULE_AUTHOR("Alessandro Cingolani");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("Main Device module");
MODULE_VERSION("0.1");