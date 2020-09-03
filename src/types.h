#ifndef TYPES_H
#define TYPES_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/workqueue.h>
#include <linux/cdev.h>


#define DEBUG   //TODO: to remove


typedef struct t_message{
    pid_t author;   //Author thread
    void *buffer;
    ssize_t size;
} msg_t;


/** @brief Threads that are recipient the group device */
typedef struct t_group_members{
    pid_t pid;
    struct list_head list;
} group_members_t;


struct t_message_deliver{
    msg_t message;

    struct list_head recipient; //PIDs of threads that should read 'message'
    struct rw_semaphore recipient_lock;

    struct list_head fifo_list;
};


typedef struct t_message_manager{
    u_int max_message_size;
    u_int max_storage_size;

    u_int curr_storage_size;
    struct rw_semaphore config_lock;


    struct list_head queue;
    struct rw_semaphore queue_lock;

} msg_manager_t;



/**
 * @brief system-wide descriptor of a group
 */
typedef struct group_t {
	unsigned int group_id;		//Thread group ID
	char *group_name;
} group_t;



typedef struct group_data {
    struct cdev cdev;           //Characted Device definition  
    dev_t deviceID;             //TODO: remove as already present in "cdev"
    int group_id;               //Returned by the IDR

    struct list_head active_members;
    atomic_t members_count;
    struct rw_semaphore member_lock;

    msg_manager_t *msg_manager;
    
    struct work_struct garbage_collector_work;

    struct device* dev;
    group_t *descriptor;        /** @brief system-wide descriptor*/
} group_data;

#endif //TYPES_H