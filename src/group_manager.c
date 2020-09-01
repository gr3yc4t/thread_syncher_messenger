#include "group_manager.h"




inline void initParticipants(group_data *grp_data){
    INIT_LIST_HEAD(&grp_data->active_members);
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

    dev_t deviceID;

    snprintf(device_name, DEVICE_NAME_SIZE, "group%d", grp_data->group_id);
    printk(KERN_DEBUG "Device name: %s", device_name);


    err = alloc_chrdev_region(&grp_data->deviceID, 1, GROUP_MAX_MINORS, device_name);

    if (err != 0) {
        printk(KERN_INFO "Unable to register 'group%d' device file", grp_data->group_id);
        return err;
    }

    printk(KERN_DEBUG "Device Major/Minor correctly allocated");

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
    grp_data->msg_manager = createMessageManager(256, 256);

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

}




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
        list_add(&newMember->list, &grp_data->active_members);
        printk("New member (%d) of group %d added", current->pid, grp_data->group_id);
    }
    
    return 0;
}


static int releaseGroup(struct inode *inode, struct file *file){

    group_data *grp_data = file->private_data;

    printk(KERN_INFO " - Group %d released by %ld - ", grp_data->group_id, current->pid);

}

/**
 * @brief Consult the message-manager and fetch msg_t structure 
 * 
 * 
 * @todo  Conversion from kernel address to user address
 */
static ssize_t readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset){
    group_data *grp_data = (group_data*) file->private_data;

    printk(KERN_DEBUG "Reading messages from group%d", grp_data->group_id);

    if(size < sizeof(msg_t)){
        pr_debug("Size must be at least %d", sizeof(msg_t));
        return -1;
    }

    msg_t message;

    if(readMessage(&message, grp_data->msg_manager)){
        printk(KERN_INFO "No message available");
        return -1;  //TODO: differentiate from normal error
    }


    printk(KERN_INFO "A message was available!!");

    //Parse the message

    if(!copy_msg_to_user(&message, (msg_t*)user_buffer)){
        printk(KERN_ERR "Unable to copy the message to user-space");
        return -1;
    }

    printk(KERN_INFO "Message copied to user-space");

    return 0;
}



static ssize_t writeGroupMessage(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos){

    group_data *grp_data = (group_data*) filep->private_data;


    if(count < sizeof(msg_t)){
        printk(KERN_INFO "Message could be delivered only in 'msg_t' format");
        return 0;
    }


    msg_t* msgTemp = (msg_t*)kmalloc(sizeof(msg_t), GFP_KERNEL);

    copy_msg_from_user(msgTemp, (msg_t*)buf); 

    int ret = writeMessage(msgTemp, grp_data->msg_manager, &grp_data->active_members);

    if(ret < 0){
        printk(KERN_ERR "Unable to write the message");
        return -1;
    }

    printk(KERN_INFO "Message for group%d queued", grp_data->group_id);

    return ret;
}