#ifndef TYPES_H
#define TYPES_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/workqueue.h>
#include <linux/cdev.h>


#ifndef DISABLE_DELAYED_MSG
    #include <linux/timer.h>
#endif

#ifndef DISABLE_THREAD_BARRIER
    #include <linux/wait.h>     //For wait-queue
    #include <linux/sched.h>
#endif



typedef struct t_message_manager msg_manager_t;
typedef struct t_message msg_t;

#define DEBUG   //TODO: to remove

/**
 * @brief Basic structure to represent a message
 */
typedef struct t_message{
    pid_t author;   //Author thread
    void *buffer;
    ssize_t size;
} msg_t;


/** @brief Threads that are members of the group device */
typedef struct t_group_members{
    pid_t pid;
    struct list_head list;
} group_members_t;

/**
 * @brief Contains a 'msg_t' structure and the relative delivery info
 * 
 * The recipient list contains the PIDs of thread that readed the message
 * 
 */
struct t_message_deliver{
    msg_t message;

    struct list_head recipient;         /**< PIDs of threads that should read 'message' */
    struct rw_semaphore recipient_lock;

    struct list_head fifo_list;
};

#ifndef DISABLE_DELAYED_MSG

struct t_message_delayed_deliver{
    msg_t message;

    msg_manager_t *manager;         //TODO: Rebase the code to remove this by using 'container_of'

    struct timer_list delayed_timer;
    struct list_head delayed_list;  
};

#endif

/**
 * @brief Manage the message sub-system
 * 
 * The 'queue' represent the FIFO list of messages, while the other members are
 *  respectively the sub-system's size limits
 */
typedef struct t_message_manager{
    u_int max_message_size;
    u_int max_storage_size;

    u_int curr_storage_size;
    struct rw_semaphore config_lock;


    struct list_head queue;
    struct rw_semaphore queue_lock;

    #ifndef DISABLE_DELAYED_MSG
        atomic_long_t message_delay;
        struct list_head delayed_queue;
        struct semaphore delayed_lock;
    #endif

} msg_manager_t;



/**
 * @brief System-wide descriptor of a group
 */
typedef struct group_t {
	unsigned int group_id;		//Thread group ID
	char *group_name;           //TODO: add name len.
} group_t;


/**
 * @brief Group device data structure
 * 
 * Contains all the modules and private data of a group character device
 * 
 * @var group_id The unique ID of the group (returned by the IDR)
 */
typedef struct group_data {
    struct cdev cdev;           /**< Characted Device definition  */
    dev_t deviceID;            
    int group_id;               /**< Unique identifier of a group. Provided by IDR */

    //Owner
    pid_t owner;

    //Members
    struct list_head active_members;    
    atomic_t members_count;
    struct rw_semaphore member_lock;

    //Message-Subsystem
    msg_manager_t *msg_manager;
    struct work_struct garbage_collector_work;

    #ifndef DISABLE_THREAD_BARRIER
        //Thread-barrier
        wait_queue_head_t barrier_queue;
        bool wake_up_flag;
    #endif

    struct device* dev;
    group_t *descriptor;        /** @brief system-wide   descriptor*/
} group_data;

#endif //TYPES_H