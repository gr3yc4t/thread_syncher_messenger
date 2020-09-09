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
int mainInit(void)
{
	int ret;

	printk(KERN_INFO "%s loading ...\n", D_DEV_NAME);

	/* register devices */
	if ((ret = sRegisterMainDev()) != 0) {
		printk(KERN_ERR "register_dev() failed\n");
		return ret;
	}

	// The main device structure will be allocated on the 
	// first "open" syscall issued on the device, so for now 
	// is marked as NULL

	main_device == NULL;	

	return 0;
}

/**
 * @brief Kernel Module Exit
 * @todo Check that every structure is correctly deallocated
 * @param nothing
 * @retval nothing
 */
void mainExit(void)
{
	printk(KERN_INFO "%s unloading ...\n", D_DEV_NAME);

	group_data *cursor1, *cursor2;
	int id_cursor1, id_cursor2;

	if(main_device == NULL){
		class_destroy(group_class);
		printk(KERN_INFO "Class destroyed");
		return;
	}

	if(group_class != NULL){

		idr_for_each_entry(&main_device->group_map, cursor1, id_cursor1){
			//unregisterGroupDevice(cursor);	//TODO:Test if this is enough
			device_destroy(group_class, cursor1->deviceID);
			pr_debug("Device %s destroyed", cursor1->descriptor->group_name);
		}


		class_destroy(group_class);
		printk(KERN_INFO "Class destroyed");

		
		idr_for_each_entry(&main_device->group_map, cursor2, id_cursor2){
			cdev_del(&cursor2->cdev);
			printk(KERN_DEBUG "Character device %s destroyed", cursor1->descriptor->group_name);
			unregister_chrdev_region(cursor2->deviceID, 1);
			printk(KERN_DEBUG "Region %s deallocated", cursor1->descriptor->group_name);

			//idr_remove(&main_device->group_map, id_cursor);
		}
	
	}


	/* unregister devices */
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

	INIT_LIST_HEAD(&main_device->groups_lst);

	idr_init(&main_device->group_map);	//Init group IDR

	sema_init(&main_device->sem, 1);	//Init main device semaphore
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

	//Check if dev was previously opened, otherwise initialize device structures
	if(main_device == NULL){	

		/* allocate main device data */
		main_device = (main_sync_t *) kmalloc(sizeof(main_sync_t), GFP_KERNEL);

		if(!main_device)
			return -1;

		initializeMainDevice();

		//Store data into 'device_info' (kept per device)
		//filep->device_info = main_device;

		//Dev should only answare to ioctl request so no private data is needed
		//Possible improvement: store into 'private_data' the creator of a group
	}

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
	dev_t dev, dev_tmp;
	int ret, i;

	/* acquire major#, minor# */
	if ((ret = alloc_chrdev_region(&dev, D_DEV_MINOR, D_DEV_NUM, D_DEV_NAME)) < 0) {
		printk(KERN_ERR "alloc_chrdev_region() failed while registering main_device\n");
		return ret;
	}

	main_dev_major = MAJOR(dev);
	main_dev_minor = MINOR(dev);

	/* create device class */
	main_class = class_create(THIS_MODULE, D_DEV_NAME);
	if (IS_ERR(main_class)) {
		return PTR_ERR(main_class);
	}

	/* allocate charactor devices */
	main_cdev = (struct cdev *)kmalloc(sizeof(struct cdev), GFP_KERNEL);

	for (i = 0; i < D_DEV_NUM; i++) {
		dev_tmp = MKDEV(main_dev_major, main_dev_minor);
		/* initialize charactor devices */
		cdev_init(main_cdev, &main_fops);
		main_cdev->owner = THIS_MODULE;
		/* register charactor devices */
		if (cdev_add(main_cdev, dev_tmp, 1) < 0) {
			printk(KERN_ERR "cdev_add() failed: minor# = %d\n", main_dev_minor);
			continue;
		}
		/* create device node */
		main_dev = device_create(main_class, NULL, dev_tmp, NULL, D_DEV_NAME "%u", main_dev_minor);
	}

	return 0;
}

/**
 * @brief Unregister Main Devices
 *
 * @param nothing
 *
 * @return nothing
 */
static void sUnregisterMainDev(void){
	dev_t dev_tmp;

	dev_tmp = MKDEV(main_dev_major, main_dev_minor);
	/* delete charactor devices */
	cdev_del(main_cdev);
	/* destroy device node */
	device_destroy(main_class, dev_tmp);


	/* release major#, minor# */
	dev_tmp = MKDEV(main_dev_major, main_dev_minor);
	unregister_chrdev_region(dev_tmp, D_DEV_NUM);

	/* destroy device class */
	class_destroy(main_class);

	/* deallocate charactor device */
	kfree(main_cdev);
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
			return -1;
		pr_debug("New 'group_t' structure allocated");

		//Copy parameter from user space
		if(ret = copy_from_user(new_group, user_buffer, sizeof(group_t))){	//Fetch the group_t structure from userspace
			printk(KERN_ERR "'group_t' structure cannot be copied from userspace; %d copied", ret);
			return -1;
		}

		printk(KERN_INFO "Installing group...");
		ret = installGroup(new_group);


		if(ret < 0){
			printk(KERN_INFO "Unable to install a group, exiting");
			kfree(new_group);
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
 * @param [in]	new_group_descriptor	The group descriptor
 */

int installGroup(const group_t *new_group_descriptor){

	group_data *new_group;
	int group_id;

	new_group = kmalloc(sizeof(group_data), GFP_KERNEL);

	if(!new_group)
		return -1;


	new_group->descriptor = new_group_descriptor;

	BUG_ON(main_device == NULL);

	down(&main_device->sem);

		//Allocate ID
		pr_debug("Allocating IDR");
		new_group->group_id  = idr_alloc(&main_device->group_map, new_group, GRP_MIN_ID, GRP_MAX_ID, GFP_KERNEL);

	up(&main_device->sem);

	if(new_group->group_id  == -ENOSPC){
		printk(KERN_INFO "Unable to allocate ID for the new group");
		goto cleanup;
	}

	pr_debug("Registering Group device...");
	int err = registerGroupDevice(new_group, main_dev);

	if(err != 0){
		printk(KERN_ERR "Error: %d", err);
		goto cleanup;
	}


	return 0;


	cleanup:
		//Free memory
		kfree(new_group);
		down(&main_device->sem);
			idr_remove(&main_device->group_map, group_id);
		up(&main_device->sem);
		return -1;
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

	list_for_each_safe(cursor, temp, &main_device->groups_lst){

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