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


#define DEVICE_NAME_SIZE    64




typedef struct t_message_manager msg_manager_t;
typedef struct t_message msg_t;

#define DEBUG   //TODO: to remove

/**
 * @brief Basic structure to represent a message
 */
typedef struct t_message{
    pid_t author;   //Author thread
    void *buffer;
    size_t size;
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

#ifndef DISABLE_SYSFS

    typedef struct t_group_sysfs {
        struct kobject *group_kobject;
        struct kobj_attribute attr_max_message_size;
        struct kobj_attribute attr_max_storage_size;
        struct kobj_attribute attr_current_storage_size;
        
        struct kobj_attribute attr_strict_mode;
        struct kobj_attribute attr_current_owner;
    }group_sysfs_t;

#endif



/**
 * @brief Manage the message sub-system
 * 
 * The 'queue' represent the FIFO list of messages, while the other members are
 *  respectively the sub-system's size limits
 */
typedef struct t_message_manager{
    u_long max_message_size;
    u_long max_storage_size;

    u_long curr_storage_size;
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
	char *group_name;           
    ssize_t name_len;
} group_t;


/**
 * @brief Contains the various flags that represent the status of the module
 * 
 * Those flags are useful since it indicates if a module was correctly initialized, in 
 *  this way when the module must be unloaded, no deallocation is performed on modules
 *  that failed.
 * 
 *  -initialized: indicates that a group has loaded all of its structures
 * 
 *  -thread_barrier_loaded: indicate that the 'thread barrier' submodule is initialized
 *  -wake_up_flag: to implement
 * 
 *  -sysfs_loaded: indicate that the 'sysfs' interface is initialized
 * 
 *  -strict_mode: 1 if the strict security mode is enabled, 0 otherwise
 */
typedef struct group_flags_t{
    unsigned int initialized:1;         //Set to 1 when the device driver is fully loaded

    #ifndef DISABLE_THREAD_BARRIER
        unsigned int thread_barrier_loaded:1;
        unsigned int wake_up_flag:1;    //TODO: switch to this on thread barrier
    #endif

    #ifndef DISABLE_SYSFS
        unsigned int sysfs_loaded:1;
    #endif

    unsigned int strict_mode:1; 
                                                   
} __attribute__((packed)) g_flags_t;    //TODO: check if "packed" improve performance



/**
 * @brief Group device data structure
 * 
 * Contains all the modules and private data of a group character device
 * 
 * @var group_id The unique ID of the group (returned by the IDR)
 */
typedef struct group_data {
    struct cdev cdev;           /** @brief Characted Device definition  */
    struct device* dev;
    dev_t deviceID;            
    int group_id;               /** @brief Unique identifier of a group. Provided by IDR */

    group_t  descriptor;        /** @brief system-wide   descriptor*/

    //Owner
    uid_t owner;                
    struct rw_semaphore owner_lock;

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


    #ifndef DISABLE_SYSFS
        group_sysfs_t group_sysfs;
    #endif


    g_flags_t flags;
} group_data;

#endif //TYPES_H