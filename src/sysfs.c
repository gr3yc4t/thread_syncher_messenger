#include "sysfs.h"


/**
 * @brief Return the 'max_message_size' param
 * @param[out] buffer The buffer where the string containing the value is written
 * 
 * @return The number of element written
 */
static ssize_t max_msg_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){ 
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_int max_msg_size;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_message_size);

        manager = group_sysfs->manager;

        if(!manager)
                return 0;

        printk(KERN_DEBUG "Locking config");

        down_read(&manager->config_lock);
                max_msg_size = manager->max_message_size;
        up_read(&manager->config_lock);

        
        return sprintf(buf, "%u\n", max_msg_size);
}


static ssize_t max_msg_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_message_size);

        manager = group_sysfs->manager;

        if(!manager)
                return 0;

        printk(KERN_DEBUG "Locking config");

        down_write(&manager->config_lock);
                ret = sscanf(buf, "%u", &manager->max_message_size);
        up_write(&manager->config_lock);
        return ret;
}

/**
 * @brief Return the 'max_storage_size' param
 * @param[out] buffer The buffer where the string containing the value is written
 * 
 * @return The number of element written
 */
static ssize_t max_storage_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_int max_storage_size;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_storage_size);

        manager = group_sysfs->manager;

        if(!manager)
                return 0;

        printk(KERN_DEBUG "Locking config");

        down_read(&manager->config_lock);
                max_storage_size = manager->max_storage_size;
        up_read(&manager->config_lock);

        
        return sprintf(buf, "%u\n", max_storage_size);
}


static ssize_t max_storage_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count){
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_storage_size);

        manager = group_sysfs->manager;

        if(!manager)
                return 0;

        printk(KERN_DEBUG "Locking config");
        down_write(&manager->config_lock);
                ret = sscanf(buf, "%u", &manager->max_storage_size);
        up_write(&manager->config_lock);
        return ret;
}

/**
 * @brief Return the 'current_storage_size' param
 * @param[out] buffer The buffer where the string containing the value is written
 * 
 * @return The number of element written
 */
static ssize_t current_storage_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf){
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_int curr_storage_size;

        group_sysfs = container_of(attr, group_sysfs_t, attr_current_storage_size);

        manager = group_sysfs->manager;

        if(!manager)
                return 0;

        printk(KERN_DEBUG "Locking config");
        down_read(&manager->config_lock);
                curr_storage_size = manager->curr_storage_size;
        up_read(&manager->config_lock);
        printk(KERN_DEBUG "Unlocking config");

        
        return sprintf(buf, "%u\n", curr_storage_size);
}

/**
 * @brief Initialize sysfs attributes
 * @param[in] grp_data Pointer to the main structure of a group
 * 
 * @return nothing
 */
void initSysFs(group_data *grp_data){

        if(!grp_data)
                return;

        struct kobject *group_device = &grp_data->cdev.kobj;     
        group_sysfs_t *sysfs = &grp_data->group_sysfs;

        sysfs->group_kobject = kobject_create_and_add("message_param", group_device);

        if(sysfs->group_kobject == NULL)
                return;

        sysfs->attr_max_message_size.attr.name = "max_message_size";
        sysfs->attr_max_message_size.attr.mode =  S_IWUGO | S_IRUGO;
        sysfs->attr_max_message_size.show = max_msg_size_show;
        sysfs->attr_max_message_size.store = max_msg_size_store;


        sysfs->attr_max_storage_size.attr.name = "max_storage_size";
        sysfs->attr_max_storage_size.attr.mode =  S_IWUGO | S_IRUGO;
        sysfs->attr_max_storage_size.show = max_storage_size_show;
        sysfs->attr_max_storage_size.store = max_storage_size_store;

        sysfs->attr_current_storage_size.attr.name = "current_message_size";
        sysfs->attr_current_storage_size.attr.mode =  S_IRUGO;
        sysfs->attr_current_storage_size.show = current_storage_size_show;



        sysfs_create_file(sysfs->group_kobject, &sysfs->attr_max_message_size.attr);
        sysfs_create_file(sysfs->group_kobject, &sysfs->attr_max_storage_size.attr);
        sysfs_create_file(sysfs->group_kobject, &sysfs->attr_current_storage_size.attr);

}


/**
 * @brief Deallocate all sysfs attributes
 * @param[in] sysfs Pointer to the 'group_sysfs_t' structure of a group
 * 
 * @return nothing
 */
void releaseSysFs(group_sysfs_t *sysfs){
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_max_message_size.attr);
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_max_storage_size.attr);
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_current_storage_size.attr);

    kobject_put(sysfs->group_kobject);
}