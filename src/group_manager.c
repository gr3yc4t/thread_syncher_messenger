/**
 * @file		group_manager.c
 * @brief		Handles all procedures releated to file operation issued on a group device
 *
 */

#include "group_manager.h"



/**
 * @brief Initialize group_data participants' structures
 * 
 * @param[in] grp_data Pointer to the device's group_data structure
 * 
 * @return nothing
 */
inline void initParticipants(group_data *grp_data){

    BUG_ON(grp_data == NULL);

    INIT_LIST_HEAD(&grp_data->active_members);
    atomic_set(&grp_data->members_count, 0);
    init_rwsem(&grp_data->member_lock);
}

/**
 * @brief Remove an element from the list of participant thread
 * @param [in] participants  Pointer to the list_head of participants
 * @param [in] _pid The element to remove
 * 
 * @return 0 on success EMPTY_LIST if the list is empty, NODE_NOT_FOUND if the 
 *      element is not present on the list 
 * 
 * @note This functions is not thread-safe, and should be procected with a lock
 */
int removeParticipant(struct list_head *participants, pid_t _pid){

    struct list_head *cursor;
    struct list_head *temp;
    group_members_t *entry;


    BUG_ON(participants == NULL);

    if(list_empty(participants)){
        return EMPTY_LIST;
    }


    list_for_each_safe(cursor, temp, participants){    //TODO: check if the unsafe version is enough

        entry = list_entry(cursor, group_members_t, list);

        if(entry->pid == _pid){
            list_del_init(cursor);
            return 0;
        }
    }

    return NODE_NOT_FOUND;
}


/**
 *  @brief register a group device 
 *  @param [in] grp_data    The group data descriptor
 *  @param [in] parent      device parent (usually 'main_device')
 * 
 *  @return 0 on success, <0 otherwise
 */
int registerGroupDevice(group_data *grp_data, const struct device* parent){

    int err;
    char device_name[DEVICE_NAME_SIZE];    //Device name buffer

    snprintf(device_name, DEVICE_NAME_SIZE, "group%d", grp_data->group_id);
    pr_debug("Device name: %s", device_name);


    err = alloc_chrdev_region(&grp_data->deviceID, 1, GROUP_MAX_MINORS, device_name);

    if (err != 0) {
        printk(KERN_INFO "Unable to register 'group%d' device file", grp_data->group_id);
        return err;
    }

    pr_debug("Device Major/Minor correctly allocated");

    cdev_init(&grp_data->cdev, &group_operation);

    err = cdev_add(&grp_data->cdev, grp_data->deviceID, 1);
    if(err < 0){
        printk(KERN_ERR "Unable to add char dev. Error %d", err);
        return err;
    }


    //Group ID is stored both on the group_t descriptor and on the generic structure
    grp_data->descriptor->group_id = grp_data->group_id;

    grp_data->descriptor->group_name = kmalloc(sizeof(char)*strnlen(device_name, DEVICE_NAME_SIZE), GFP_KERNEL);
    strcpy(grp_data->descriptor->group_name, device_name);


    //Create "group_sync" on the first device creation
    if(group_class == NULL){
        group_class = class_create(THIS_MODULE, "group_sync");

        if(IS_ERR(group_class)){

            if(group_class == -EEXIST){
                printk(KERN_INFO "'group_sync' class already exists, skipping class creation");
                //group_class = class_find("group_sync");
            }else{
                printk(KERN_ERR "Unable to create 'group_sync' class");
                goto cleanup;
            }
        }
    }
    
    //TODO: test parent behaviour
    grp_data->dev = device_create(group_class, NULL, grp_data->deviceID, NULL, device_name);

    if(IS_ERR(grp_data->dev)){
        printk(KERN_ERR "Unable to register the device");
        goto cleanup;
    }

    printk(KERN_INFO "Device correctly added");

    //Initialize linked-list
    initParticipants(grp_data);   
    //Initialize Message Manager  
    grp_data->msg_manager = createMessageManager(DEFAULT_MSG_SIZE, DEFAULT_STORAGE_SIZE, &grp_data->garbage_collector_work);
    
    #ifndef DISABLE_THREAD_BARRIER
        //Initialize Wait Queue
        init_waitqueue_head(&grp_data->barrier_queue);
        grp_data->wake_up_flag = false;
    #endif


    return 0;

    //----------------------------------------------------------
    cleanup:
        cdev_del(&grp_data->cdev);  //Free the cdev device
        kfree(grp_data->descriptor->group_name);
        return -1;
}

/**
 *  @brief unregister a group device
 *  @param [in] grp_data    The group data descriptor
 *  @return nothing
 */

void unregisterGroupDevice(group_data *grp_data){


    printk(KERN_INFO "Cleaning up 'group%d'", grp_data->group_id);
    cdev_del(&grp_data->cdev);

    BUG_ON(group_class == NULL);

    device_destroy(group_class, grp_data->deviceID);
    
    unregister_chrdev_region(grp_data->deviceID, 1);    //TODO: check the 1

    #ifndef DISABLE_THREAD_BARRIER
        //Waking up all the sleeped thread
        printk(KERN_INFO "Waking up all the sleeped thread 'group%d'", grp_data->group_id);
        grp_data->wake_up_flag = true;
    #endif
}



/**
 * @brief Called when an application opens the device file
 * 
 * Add the process that opened the device to the 'active_members' list
 * 
 */
static int openGroup(struct inode *inode, struct file *file){
    group_data *grp_data;

    grp_data = container_of(inode->i_cdev, group_data, cdev);

    file->private_data = grp_data;

    printk(KERN_DEBUG "Group %d opened", grp_data->group_id);

    //TODO: integrate in a function
    {
        group_members_t *newMember = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);
        if(!newMember){
            printk(KERN_ERR "Unable to allocate new member");
            return -1;
        }

        newMember->pid = current->pid;
        
        down_write(&grp_data->member_lock);
            list_add(&newMember->list, &grp_data->active_members);
        up_write(&grp_data->member_lock);

        atomic_inc(&grp_data->members_count);
        printk("New member (%d) of group %d added", current->pid, grp_data->group_id);
    }
    
    return 0;
}

/**
 * @brief Called when an application closes the device file
 * 
 * Remove the process that closed the device from the 'active_members' list
 *  and start the garbage collector
 * 
 */
static int releaseGroup(struct inode *inode, struct file *file){
    group_data *grp_data;
    
    
    grp_data =  (group_data*)file->private_data;

    printk(KERN_INFO " - Group %d released by %d - ", grp_data->group_id, current->pid);

    down_write(&grp_data->member_lock);
        int ret = removeParticipant(&grp_data->active_members, current->pid);
    up_write(&grp_data->member_lock);

    if(ret == EMPTY_LIST){
        printk(KERN_ERR "Releasig group, strange error: EMPTY_LIST");
        return 0;
    }
    if(ret == NODE_NOT_FOUND){
        printk(KERN_ERR "Releasig group, strange error: NODE_NOT_FOUND");
        return 0;
    }

    atomic_dec(&grp_data->members_count);

    pr_debug("Removed participant %d from active members", current->pid);


    //Start a workqueue for cleaning up message that are completely delivered
    schedule_work(&grp_data->garbage_collector_work);

    return 0;

}

/**
 * @brief Consult the message-manager and fetch incoming structure 
 * 
 * @note 'offset' is ignored since messages are independent data unit
 * @note If more byte than the available is requested, the function only copies the 
 *          available bytes.
 * @return The number of bytes readed
 */
static ssize_t readGroupMessage(struct file *file, char __user *user_buffer, size_t _size, loff_t *offset){
    group_data *grp_data;
    msg_t message;
    ssize_t available_size;


    grp_data = (group_data*) file->private_data;

    printk(KERN_DEBUG "Reading messages from group%d", grp_data->group_id);

    if(readMessage(&message, grp_data->msg_manager)){
        printk(KERN_INFO "No message available");
        return NO_MSG_PRESENT;
    }


    printk(KERN_INFO "A message was available!!");

    //If the user-space application request more byte than available, return only available bytes
    if(message.size < _size){
        available_size = message.size;
    }else{
        available_size = _size;
    }


    //Parse the message
    if(copy_msg_to_user(&message, (int8_t*)user_buffer, available_size) == -EFAULT){
        printk(KERN_ERR "Unable to copy the message to user-space");
        return MEMORY_ERROR;
    }

    printk(KERN_INFO "Message copied to user-space");

    pr_debug("Scheduling garbage collector");
    //Start a workqueue for cleaning up message that are completely delivered
    schedule_work(&grp_data->garbage_collector_work);

    return available_size;
}

/**
 * @brief Routine called when a 'write()' is issued on the group char device
 * 
 * @return 0 on success, MSG_SIZE_ERROR if the size of message is wrong, 
 */

static ssize_t writeGroupMessage(struct file *filep, const char __user *buf, size_t _size, loff_t *f_pos){
    int ret;
    group_data *grp_data;
    
    grp_data = (group_data*) filep->private_data;

    msg_t* msgTemp = (msg_t*)kmalloc(sizeof(msg_t), GFP_KERNEL);

    if(!msgTemp)
        return 0;

    copy_msg_from_user(msgTemp, (int8_t*)buf, _size); 

    msgTemp->author = current->pid;
    msgTemp->size = _size;

    #ifndef DISABLE_DELAYED_MSG

        if(isDelaySet(grp_data->msg_manager)){
            ret = queueDelayedMessage(msgTemp, grp_data->msg_manager);
        }else
            ret = writeMessage(msgTemp, grp_data->msg_manager);
        
    #else
        ret = writeMessage(msgTemp, grp_data->msg_manager);
    #endif

    if(ret < 0){
        printk(KERN_ERR "Unable to write the message");
        return -1;
    }

    printk(KERN_INFO "Message for group%d queued", grp_data->group_id);

    return ret;
}



static int flushGroupMessage(struct file *filep, fl_owner_t id){
    group_data *grp_data;
    msg_manager_t *manager;
    int ret;

    grp_data = (group_data*) filep->private_data;
    manager = grp_data->msg_manager;

    if(atomic_long_read(&manager->message_delay) == 0)
        return 0;

    ret = cancelDelay(manager);

    printk(KERN_INFO "flush: %d elements flushed from the delayed queue", ret);
    return ret;
}


#ifndef DISABLE_THREAD_BARRIER
/**
 * @brief Resets the flag used to wake up threds
 * @param[in] grp_data Pointer to the main structure group
 * 
 * @return true if the flag was resetted, false otherwise
 */
bool resetSleepFlag(group_data *grp_data){
    bool ret_flag = false;

    spin_lock(&grp_data->barrier_queue.lock);
        if(!waitqueue_active(&grp_data->barrier_queue)){
            printk(KERN_INFO "resetSleepFlag: wake_up_flag resetted");
            grp_data->wake_up_flag = false;
            ret_flag = true;
        }
    spin_unlock(&grp_data->barrier_queue.lock);

    return ret_flag;
}


/**
 * @brief Put the thread which calles this function on sleep
 * 
 * @param[in] grp_data Pointer to the main structure of a group
 * 
 * @return nothing
 */
void sleepOnBarrier(group_data *grp_data){
    
    if(resetSleepFlag(grp_data)){
        printk(KERN_INFO "First thread inserted into the queue");
    }else{
        printk(KERN_INFO "Sleep flag already set to 'false'");
    }

    printk(KERN_INFO "Putting thread %d to sleep...", current->pid);
    wait_event(grp_data->barrier_queue, grp_data->wake_up_flag == true);

    printk(KERN_INFO "Thread %d woken up!!", current->pid);
    schedule();

}

/**
 * @brief Wake up all threads that was put on sleep by the module
 * @param[in] grp_data Pointer to the main structure of a group
 * 
 * @return nothing
 */
void awakeBarrier(group_data *grp_data){

    printk(KERN_INFO "Waking up threads in the barrier_queue");

    grp_data->wake_up_flag = true;
    wake_up_all(&grp_data->barrier_queue);

    //resetSleepFlag(grp_data);

    printk(KERN_INFO "Queue waked up");

    return;
}

#endif


/**
 * 
 * 
 * 
 * 
 * 
 * 
 */
long int groupIoctl(struct file *filep, unsigned int ioctl_num, unsigned long ioctl_param){

	int ret;
    long delay = 0;
    group_data *grp_data;


	switch (ioctl_num){
        #ifndef DISABLE_DELAYED_MSG
            case IOCTL_SET_SEND_DELAY:
                delay = (long) ioctl_param;

                if(delay < 0)   //Fix conversion issue
                    delay = 0;

                grp_data = (group_data*) filep->private_data;

                atomic_long_set(&grp_data->msg_manager->message_delay, delay);

                printk(KERN_INFO "Message delay set to: %ld", delay);
                ret = 0;
                break;
            case IOCTL_REVOKE_DELAYED_MESSAGES:

                grp_data = (group_data*) filep->private_data;

                ret = revokeDelayedMessage(grp_data->msg_manager);

                printk(KERN_INFO "Revoke Message: %d message revoked from queue", ret);

                break;
        #endif
        #ifndef DISABLE_THREAD_BARRIER
            case IOCTL_SLEEP_ON_BARRIER:
                grp_data = (group_data*) filep->private_data;
                printk(KERN_INFO "Sleeping call issued");
                sleepOnBarrier(grp_data);
                ret = 0;
                break;
            case IOCTL_AWAKE_BARRIER:
                grp_data = (group_data*) filep->private_data;
                printk(KERN_INFO "Awaking call issued");
                awakeBarrier(grp_data);
                ret = 0;
                break;
        #endif
	default:
		printk(KERN_INFO "Invalid IOCTL command provided: \n\tioctl_num=%u\n\tparam: %lu", ioctl_num, ioctl_param);
		ret = -1;
		break;
	}

	return ret;
}