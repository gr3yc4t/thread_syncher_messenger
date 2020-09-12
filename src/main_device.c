#include "main_device.h"


/** file operations */
static struct file_operations main_fops = {
	.open    = mainOpen,
	.release = mainRelease,
	.write   = mainWrite,
	.read    = mainRead,
	.unlocked_ioctl = mainDeviceIoctl
};


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

	group_class == NULL;	//Will be allocated when the first group is installed

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
 */
void mainExit(void){
	printk(KERN_INFO "%s unloading ...\n", D_DEV_NAME);

	group_data *cursor;
	int id_cursor;


	printk(KERN_INFO "Class destroyed");


	if(group_class != NULL){   //If some groups were installed

		idr_for_each_entry(&main_device_data.group_map, cursor, id_cursor){
			unregisterGroupDevice(cursor);	//TODO:Test if this is enough
			printk(KERN_INFO"Device %s destroyed", cursor->descriptor->group_name);
			
			kfree(cursor);	//Deallocate group_data structure
		}


		class_destroy(group_class);
		printk(KERN_INFO "Class destroyed");

		//Deallocate the IDR
		idr_destroy(&main_device_data.group_map);
	}


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
static int mainOpen(struct inode *inode, struct file *filep)
{
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
static ssize_t mainWrite(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos)
{
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


	/* destroy device class */
	class_destroy(main_class);

	unregister_chrdev_region(main_device_data.dev, 1);

}



static long int mainDeviceIoctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){

	int ret;

	switch (ioctl_num){
	case IOCTL_INSTALL_GROUP:

		printk(KERN_INFO "INSTALL GROUP ioctl issued");

		group_t *user_buffer;
		group_t *new_group;


		user_buffer = (group_t*) ioctl_param;

		//Allocate the new structure
		new_group = kmalloc(sizeof(group_t), GFP_KERNEL);
		if(!new_group)
			return ALLOC_ERR;

		pr_debug("New 'group_t' structure allocated");

		//Copy parameter from user space
		if(ret = copy_from_user(new_group, user_buffer, sizeof(group_t))){	//Fetch the group_t structure from userspace
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return USER_COPY_ERR;
		}

		printk(KERN_INFO "Installing group...");
		ret = installGroup(new_group);


		if(ret < 0){
			printk(KERN_INFO "Unable to install a group, exiting");
			kfree(new_group);
			return ret;
		}


		printk(KERN_INFO "Group installed correctly");

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

int installGroup(const group_t *new_group_descriptor){

	group_data *new_group;
	int group_id;
	int ret = 0;

	//Try to install the 'group_class'
	if(installGroupClass() < 0)
		return CLASS_EXISTS;


	new_group = kmalloc(sizeof(group_data), GFP_KERNEL);

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

	pr_debug("Registering Group device...");
	int err = registerGroupDevice(new_group, main_device);

	if(err != 0){
		printk(KERN_ERR "Error: %d", err);
		ret = err;
		goto cleanup;
	}


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
 * @brief Check if a given group already exists
 * 
 * @param[in] group A pointer to a 'group_t' strucuture to check
 * @return true if the group exists, false otherwise
 * 
 * @note Only the 'group_id' field is used in the check, 'group_name' is ignored
 */
bool groupExists(group_t *group){

	struct list_head *cursor = NULL;	//Cursor
	struct list_head *temp;
	group_t curr_group;

	list_for_each_safe(cursor, temp, &main_device_data.groups_lst){

		group_list_t *elem = list_entry(cursor, group_list_t, list);

		curr_group = elem->group;

		if(curr_group.group_id == group->group_id)
			return true; 

	}

	return false;
}






MODULE_AUTHOR("Alessandro Cingolani");
MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("Main Device module");
MODULE_VERSION("0.1");