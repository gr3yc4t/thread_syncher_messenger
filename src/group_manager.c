#include "group_manager.h"



int registerGroupDevice(unsigned int group_id, group_data *grp_data){

    int err;
    char device_name[DEVICE_NAME_SIZE];    //Device name buffer


    snprintf(device_name, DEVICE_NAME_SIZE, "group%d\0", group_id);
    printk(KERN_DEBUG "Device name: %s", device_name);


    err = alloc_chrdev_region(&grp_data->deviceID, 1, GROUP_MAX_MINORS, device_name);

    if (err != 0) {
        printk(KERN_INFO "Unable to register 'group%d' device file", group_id);
        return err;
    }

    printk(KERN_DEBUG "Device Major/Minor correctly allocated");

    cdev_init(&grp_data->cdev, &group_operation);
    cdev_add(&grp_data->cdev, grp_data->deviceID, 1);

    printk(KERN_DEBUG "Device Added");

    grp_data->descriptor->group_id = group_id;

    grp_data->descriptor->group_name = kmalloc(sizeof(char)*strnlen(device_name, DEVICE_NAME_SIZE), GFP_KERNEL);
    strcpy(grp_data->descriptor->group_name, device_name);

    return 0;
}



int unregisterGroupDevice(unsigned int group_id){



}




static int openGroup(struct inode *inode, struct file *file){
    group_t *group_data;


}


static int readGroupMessage(struct file *file, char __user *user_buffer, size_t size, loff_t *offset){
    group_t *group_data;

}