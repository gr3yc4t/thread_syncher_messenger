/**
 * @file sysfs.h
 * @brief Setup the required sysfs interfaces for the module
 * 
 */
#ifndef SYSFS_H
#define SYSFS_H


#include <linux/fs.h> /* Header for the Linux file system support */
#include <linux/kobject.h>
#include <uapi/linux/stat.h> /* S_IRUSR, S_IWUSR  */

#include <linux/kernel.h>	/* TODO: For printk, remove after debugging*/
#include <linux/rwsem.h>
#include <linux/uaccess.h>   //For copy_to_user/copy_from_user

#include "types.h"



#define ATTR_BUFF_SIZE 64



static ssize_t max_msg_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t max_msg_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t max_storage_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t max_storage_size_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t current_storage_size_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);



int initSysFs(group_data *grp_data);
void releaseSysFs(group_sysfs_t *sysfs);




#endif //SYSFS_H