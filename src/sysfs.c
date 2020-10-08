#include "sysfs.h"

#include <linux/module.h>

/**
 * @brief Check if the current thread has the privilege to change sysfs parameters
 * 
 * @param[in] grp_data A pointer to a group_data structure
 * 
 * @retval true If the current thread has privilege
 * @retval false If the current thread has not the privilege
 */
bool hasStorePrivilege(group_data *grp_data){

        uid_t current_owner;
        bool ret;

        //If strict mode is disabled anyone can set parametrs
        if(grp_data->flags.strict_mode == 0)
                return true;

        //Otherwise, only the owner can
        down_read(&grp_data->owner_lock);

                current_owner = grp_data->owner;

                pr_debug("Current owner: %d", current_owner);
                pr_debug("Current thread: %d", current_uid().val );

                if(current_uid().val == current_owner){
                        ret = true;
                }else{
                        ret = false;
                }

        up_read(&grp_data->owner_lock);

        return ret;
}



/**
 * @brief Return the 'max_message_size' param
 * @param[out] buffer The buffer where the string containing the value is written
 * 
 * @return The number of element written
 */
static ssize_t max_msg_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){ 
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_long max_msg_size;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_message_size);

        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        manager = grp_data->msg_manager;

        if(!manager){
                printk(KERN_ERR "Unable to fetch msg_manager pointer");
                return -1;
        }

        pr_debug("Locking config");
        down_read(&manager->config_lock);
                max_msg_size = manager->max_message_size;
        up_read(&manager->config_lock);
        pr_debug("Unlocking config");

        
        return snprintf(user_buff, ATTR_BUFF_SIZE,"%ld", max_msg_size);
}

/**
 * @brief Store the 'max_message_size' param
 * @param[in] buf The buffer where the string containing the value is readed
 * 
 * @return The new parameter value, 0 if the no changes are done
 */
static ssize_t max_msg_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *user_buf, size_t count){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_long tmp;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_message_size);

        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_WARNING "container_of: error");
                return -1;
        }


        if(!hasStorePrivilege(grp_data)){
                printk(KERN_ERR "Unable to change param: unauthorized");
                return -1;
        }


        manager = grp_data->msg_manager;

        if(!manager)
                return 0;

        ret = sscanf(user_buf, "%ld", &tmp);

        if(ret < 0){
                pr_debug("Conversion error, exiting...");
                return -1;
        }

        down_write(&manager->config_lock);
                manager->max_message_size = tmp;
        up_write(&manager->config_lock);

        pr_debug("Value of 'max_msg_size' set to %ld", manager->max_message_size);

        return ret;
}

/**
 * @brief Return the 'max_storage_size' param
 * @param[out] buffer The buffer where the string containing the value is written
 * 
 * @return The number of element written
 */
static ssize_t max_storage_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_long max_storage_size;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_storage_size);

        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        manager = grp_data->msg_manager;

        if(!manager){
                printk(KERN_ERR "Unable to fetch msg_manager pointer");
                return -1;
        }

        down_read(&manager->config_lock);
                max_storage_size = manager->max_storage_size;
        up_read(&manager->config_lock);

        return snprintf(user_buff, ATTR_BUFF_SIZE,"%ld", max_storage_size);
}

/**
 * @brief Store the 'max_storage_size' param
 * @param[in] buf The buffer where the string containing the value is readed
 * 
 * @return The new parameter value, 0 if the no changes are done
 */
static ssize_t max_storage_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *user_buf, size_t count){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_long tmp;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_max_storage_size);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);

        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        if(!hasStorePrivilege(grp_data)){
                printk(KERN_ERR "Unable to change param: unauthorized");
                return -1;
        }

        manager = grp_data->msg_manager;

        if(!manager)
                return -1;

        ret = sscanf(user_buf, "%ld", &tmp);

        if(ret < 0){
                pr_debug("Conversion error, exiting...");
                return -1;
        }

        down_write(&manager->config_lock);
                manager->max_storage_size = tmp;
        up_write(&manager->config_lock);

        pr_debug("Value of 'max_storage_size' set to %ld", manager->max_storage_size);

        return 0;
}

/**
 * @brief Return the 'current_storage_size' param
 * @param[out] buffer The buffer where the string containing the value is written
 * 
 * @return The number of element written
 */
static ssize_t current_storage_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        msg_manager_t *manager;
        u_long curr_storage_size;

        group_sysfs = container_of(attr, group_sysfs_t, attr_current_storage_size);

        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        manager = grp_data->msg_manager;

        if(!manager){
                printk(KERN_ERR "Unable to fetch msg_manager pointer");
                return -1;
        }

        down_read(&manager->config_lock);
                curr_storage_size = manager->curr_storage_size;
        up_read(&manager->config_lock);


        return snprintf(user_buff, ATTR_BUFF_SIZE,"%ld", curr_storage_size);
}


/**
 * @brief Return the current owner of a group
 * @param[out] buffer The buffer where the string containing the owner's PID is written
 * 
 * @return The number of element written
 */
static ssize_t current_owner_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        uid_t curr_owner;

        group_sysfs = container_of(attr, group_sysfs_t, attr_current_storage_size);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }


        down_read(&grp_data->owner_lock);
                curr_owner = grp_data->owner;
        up_read(&grp_data->owner_lock);


        return snprintf(user_buff, ATTR_BUFF_SIZE, "%u", (unsigned int)curr_owner);
}

/**
 * @brief Return the current owner of a group
 * @param[out] buffer The buffer where the string containing the owner's PID is written
 * 
 * @return The number of element written
 */
static ssize_t strict_mode_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        bool enabled;

        group_sysfs = container_of(attr, group_sysfs_t, attr_current_storage_size);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        if(grp_data->flags.strict_mode == 1)
                enabled = true;
        else
                enabled = false;
        
        return snprintf(user_buff, ATTR_BUFF_SIZE,"%d", enabled);
}

/**
 * @brief Return the current owner of a group
 * @param[out] buffer The buffer where the string containing the owner's PID is written
 * 
 * @return The number of element written
 */
static ssize_t garbage_collector_enabled_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        bool enabled;

        group_sysfs = container_of(attr, group_sysfs_t, attr_garbage_collector_enabled);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        if(grp_data->flags.garbage_collector_disabled == 1)
                enabled = false;
        else
                enabled = true;
        
        return snprintf(user_buff, ATTR_BUFF_SIZE,"%d", enabled);
}


/**
 * @brief Store the 'max_storage_size' param
 * @param[in] buf The buffer where the string containing the value is readed
 * 
 * @return The new parameter value, 0 if the no changes are done
 */
static ssize_t garbage_collector_enabled_store(struct kobject *kobj, struct kobj_attribute *attr, const char *user_buf, size_t count){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        int tmp;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_garbage_collector_enabled);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);

        if(!grp_data){
                pr_err("container_of: error");
                return -1;
        }

        if(!hasStorePrivilege(grp_data)){
                pr_err("Unable to change parameter: unauthorized");
                return -1;
        }


        ret = sscanf(user_buf, "%d", &tmp);

        if(ret < 0){
                pr_debug("Conversion error, exiting...");
                return -1;
        }

        if(tmp > 0)
                grp_data->flags.garbage_collector_disabled = 0;
        else if(tmp == 0)
                grp_data->flags.garbage_collector_disabled = 1;
        else
                return -1;
        
        pr_debug("Garbage collector flag set to: %d", grp_data->flags.garbage_collector_disabled);

        return 0;
}




/**
 * @brief Return the current ratio of a group's garbage collector
 * @param[out] buffer The buffer where the string containing the owner's PID is written
 * 
 * @return The number of element written
 */
static ssize_t garbage_collector_ratio_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        int ratio;

        group_sysfs = container_of(attr, group_sysfs_t, attr_garbage_collector_ratio);

        if(!group_sysfs)
                return -1;

        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        ratio = atomic_read(&grp_data->garbage_collector.ratio);
        
        return snprintf(user_buff, ATTR_BUFF_SIZE,"%d", ratio);
}


/**
 * @brief Store the 'ratio' param of a grop's garbage collector
 * @param[in] buf The buffer where the string containing the value is readed
 * 
 * @return The new parameter value, 0 if the no changes are done
 */
static ssize_t garbage_collector_ratio_store(struct kobject *kobj, struct kobj_attribute *attr, const char *user_buf, size_t count){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        int tmp;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_garbage_collector_ratio);
        if(!group_sysfs){
                return -1;
        }
       
       grp_data = container_of(group_sysfs, group_data, group_sysfs);

        if(!grp_data){
                printk(KERN_ERR "container_of: error");
                return -1;
        }

        if(!hasStorePrivilege(grp_data)){
                printk(KERN_ERR "Unable to change parameter: unauthorized");
                return -1;
        }


        ret = sscanf(user_buf, "%d", &tmp);

        if(ret < 0){
                pr_debug("Conversion error, exiting...");
                return -1;
        }

        atomic_set(&grp_data->garbage_collector.ratio, tmp);
        
        pr_debug("Garbage collector ratio set to: %d", tmp);

        return 0;
}



/**
 * @brief Return the value of the flag used to include supporting structures in msg. size
 * @param[out] buffer The buffer where the string containing flag's value is written
 * 
 * @return The number of element written
 */
static ssize_t include_struct_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *user_buff){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        bool enabled;

        group_sysfs = container_of(attr, group_sysfs_t, attr_current_storage_size);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);
        
        if(!grp_data){
                pr_err("container_of: error");
                return -1;
        }

        if(grp_data->flags.gc_include_struct == 1)
                enabled = true;
        else
                enabled = false;
        
        return snprintf(user_buff, ATTR_BUFF_SIZE,"%d", enabled);
}



/**
 * @brief Set the value of the messages size flag
 * @param[in] buffer The buffer containing the new flag value
 * 
 * @return The number of element written
 */
static ssize_t include_struct_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *user_buf, size_t count){
        group_data *grp_data;
        group_sysfs_t *group_sysfs;
        int tmp;
        int ret;

        group_sysfs = container_of(attr, group_sysfs_t, attr_include_struct_size);
        grp_data = container_of(group_sysfs, group_data, group_sysfs);

        if(!grp_data){
                pr_err("container_of: error");
                return -1;
        }

        if(!hasStorePrivilege(grp_data)){
                pr_err("Unable to change parameter: unauthorized");
                return -1;
        }


        ret = sscanf(user_buf, "%d", &tmp);

        if(ret < 0){
                pr_debug("Conversion error, exiting...");
                return -1;
        }

        if(tmp > 0)
                grp_data->flags.gc_include_struct = 0;
        else if(tmp == 0)
                grp_data->flags.gc_include_struct = 1;
        else
                return -1;
        
        pr_debug("Structure message size flag set to: %d", grp_data->flags.garbage_collector_disabled);

        return 0;
}




/**
 * @brief Initialize sysfs attributes
 * @param[in] grp_data Pointer to the main structure of a group
 * 
 * @note In case the file's attribute cannot be created, no error is returned
 * 
 * @return 0 on success, negative number on error
 */
int initSysFs(group_data *grp_data){
        struct kobject *group_device;
        group_sysfs_t *sysfs;

        if(!grp_data)
                return -1;


        
        group_device = &grp_data->dev->kobj;   

        if(group_device == NULL){
                printk(KERN_WARNING "sysfs: kobject parent is NULL, trying alternative");
                group_device = &THIS_MODULE->mkobj.kobj;
        }


        sysfs = &grp_data->group_sysfs;

        sysfs->group_kobject = kobject_create_and_add("group_parameters", group_device);

        if(sysfs->group_kobject == NULL){
                pr_err("sysfs: Unable to create kobject");
                return -1;
        }
        

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

        sysfs->attr_current_owner.attr.name = "current_owner";
        sysfs->attr_current_owner.attr.mode =  S_IRUGO;
        sysfs->attr_current_owner.show = current_owner_show;

        sysfs->attr_strict_mode.attr.name = "strict_mode";
        sysfs->attr_strict_mode.attr.mode =  S_IRUGO;
        sysfs->attr_strict_mode.show = strict_mode_show;

        sysfs->attr_garbage_collector_enabled.attr.name = "garbage_collector_enabled";
        sysfs->attr_garbage_collector_enabled.attr.mode =  S_IWUGO | S_IRUGO;
        sysfs->attr_garbage_collector_enabled.show = garbage_collector_enabled_show;
        sysfs->attr_garbage_collector_enabled.store = garbage_collector_enabled_store;

        sysfs->attr_garbage_collector_ratio.attr.name = "garbage_collector_ratio";
        sysfs->attr_garbage_collector_ratio.attr.mode =  S_IWUGO | S_IRUGO;
        sysfs->attr_garbage_collector_ratio.show = garbage_collector_ratio_show;
        sysfs->attr_garbage_collector_ratio.store = garbage_collector_ratio_store;


        sysfs->attr_include_struct_size.attr.name = "include_struct_size";
        sysfs->attr_include_struct_size.attr.mode =  S_IWUGO | S_IRUGO;
        sysfs->attr_include_struct_size.show = include_struct_size_show;
        sysfs->attr_include_struct_size.store = include_struct_size_store;
;

        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_max_message_size.attr) < 0)
                printk(KERN_WARNING "Unable to create 'max_message_size' attribute");
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_max_storage_size.attr) < 0)
                printk(KERN_WARNING "Unable to create 'max_storage_size' attribute");        
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_current_storage_size.attr) < 0)
                printk(KERN_WARNING "Unable to create 'max_current_size' attribute");        
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_garbage_collector_ratio.attr) < 0)
                printk(KERN_WARNING "Unable to create 'garbage_collector_ratio' attribute");        
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_strict_mode.attr) < 0)
                printk(KERN_WARNING "Unable to create 'strict_mode' attribute");   
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_current_owner.attr) < 0)
                printk(KERN_WARNING "Unable to create 'current_owner' attribute");  
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_garbage_collector_enabled.attr) < 0)
                printk(KERN_WARNING "Unable to create 'garbage_collector_enabled' attribute");        
        if(sysfs_create_file(sysfs->group_kobject, &sysfs->attr_include_struct_size.attr) < 0)
                printk(KERN_WARNING "Unable to create 'include_struct_size' attribute");        



        return 0;
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
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_strict_mode.attr);
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_current_owner.attr);
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_garbage_collector_enabled.attr);
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_garbage_collector_ratio.attr);
    sysfs_remove_file(sysfs->group_kobject, &sysfs->attr_include_struct_size.attr);


    kobject_put(sysfs->group_kobject);

    pr_debug("syfs released");
}