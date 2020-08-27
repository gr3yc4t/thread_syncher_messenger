#include "group_manager.h"



int registerGroupDevice(group_data *grp_data, struct device* parent){

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
    cdev_add(&grp_data->cdev, deviceID, 1);


    //Group ID is stored both on the group_t descriptor and on the generic structure
    grp_data->descriptor->group_id = grp_data->group_id;

    grp_data->descriptor->group_name = kmalloc(sizeof(char)*strnlen(device_name, DEVICE_NAME_SIZE), GFP_KERNEL);
    strcpy(grp_data->descriptor->group_name, device_name);


    //Create "group_sync" on the first device creation
    if(group_class == NULL){
        group_class = class_create(THIS_MODULE, "group_sync");

        if(IS_ERR(group_class)){
            printk(KERN_ERR "Unable to create 'group_sync' class");
            goto cleanup;
        }
    }
    

    grp_data->dev = device_create(group_class, parent, grp_data->cdev.dev, NULL, device_name);

    printk(KERN_DEBUG "Device Added");


    if(IS_ERR(grp_data->dev)){
        printk(KERN_ERR "Unable to register the device");
        goto cleanup;
    }


    return 0;

    //----------------------------------------------------------
    cleanup:
        cdev_del(&grp_data->cdev);  //Free the cdev device
        kfree(grp_data->descriptor->group_name);
        return -1;
}



void unregisterGroupDevice(group_data *grp_data){


    printk(KERN_INFO "Cleaning up 'group%d'", grp_data->group_id);
    cdev_del(&grp_data->cdev);

    BUG_ON(group_class == NULL);

    device_destroy(group_class, grp_data->deviceID);
    
    unregister_chrdev_region(grp_data->deviceID, 1);    //TODO: check the 1

}




static int openGroup(struct inode *inode, struct file *file){
    //group_t *group_data;

    return 0;
}


static int readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset){
    //group_t *group_data;

    return 0;
}