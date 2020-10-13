/**
 * @file types.h
 * 
 * @brief Defines all structures and global variables needed by the module
 * 
 */


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


#define DEVICE_NAME_SIZE    64      /**< Maximum device name lenght*/




typedef struct t_message_manager msg_manager_t;
typedef struct t_message msg_t;

#define DEBUG   //TODO: to remove

/**
 * @brief Basic structure to represent a message
 */
typedef struct t_message{
    pid_t author;           /**< Process which wrote the message*/
    void *buffer;           /**< Buffer containing the message*/
    size_t size;            /**< Size (in bytes) of the buffer*/
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
    msg_t message;                          /**< The message to deliver */

    struct list_head recipient;             /**< PIDs of threads that have read 'message' */
    struct rw_semaphore recipient_lock;     /**< Recipient list semaphore*/

    struct list_head fifo_list;
};

#ifndef DISABLE_DELAYED_MSG
    /**
     * @brief Contains a 'msg_t' structure and the timer needed to delay the delivery
     * 
     * @todo Remove the 'manager' field and retrieve it at runtime (saves 8 bytes of mem.)
     */
    struct t_message_delayed_deliver{
        msg_t message;                      /**< The message to deliver*/

        msg_manager_t *manager;             /**< Pointer to the group's message manager struct */

        struct timer_list delayed_timer;    /**< The timer used to delay the delivery*/
        struct list_head delayed_list;  
    };

#endif

#ifndef DISABLE_SYSFS
    /**
     * @brief Contains all the 'sysfs' attrubutes and the group's kobject
     * 
     */
    typedef struct t_group_sysfs {
        struct kobject *group_kobject;
        struct kobj_attribute attr_max_message_size;
        struct kobj_attribute attr_max_storage_size;
        struct kobj_attribute attr_current_storage_size;
        struct kobj_attribute attr_strict_mode;
        struct kobj_attribute attr_current_owner;
        struct kobj_attribute attr_garbage_collector_enabled;
        struct kobj_attribute attr_garbage_collector_ratio;
        struct kobj_attribute attr_include_struct_size;
    }group_sysfs_t;

#endif



/**
 * @brief Manage the message sub-system
 * 
 * The 'queue' represent the FIFO list of messages, while the other members are
 *  respectively the sub-system's size limits
 */
typedef struct t_message_manager{
    u_long max_message_size;                /**< Group's max message size*/
    u_long max_storage_size;                /**< Max group storage size */

    u_long curr_storage_size;               /**< Stores the current group's messages size*/
    struct rw_semaphore config_lock;        /**< Semaphore for size parameters access*/


    struct list_head queue;                 /**< The messages FIFO queue */
    struct rw_semaphore queue_lock;         /**< FIFO queue semaphore */

    #ifndef DISABLE_DELAYED_MSG
        atomic_long_t message_delay;        /**< Specifies the current delay applied to the group*/
        struct list_head delayed_queue;     /**< The delayed messages queue*/
        struct semaphore delayed_lock;      /**< Semaphore to manage access to the 'delayed_queue'*/
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
 * this way when the module must be unloaded, no deallocation is performed on modules
 * that failed.
 * 
 *  - initialized: indicates that a group has loaded all of its structures
 *  - thread_barrier_loaded: indicate that the 'thread barrier' submodule is initialized
 *  - wake_up_flag: to implement
 *  - sysfs_loaded: indicate that the 'sysfs' interface is initialized
 *  - garbage_collector_disabled: specifies the status of the garbage collector
 *  - sysfs_loaded: indicate that the 'sysfs' interface is initialized
 *  - strict_mode: 1 if the strict security mode is enabled, 0 otherwise
 *  @todo Check the performace impact of "packing" the structure
 * 
 */
typedef struct group_flags_t{
    unsigned int initialized:1;         /**< Set to 1 when the device driver is fully loaded*/

    #ifndef DISABLE_THREAD_BARRIER
        unsigned int thread_barrier_loaded:1;   /**< 1 if the 'thread_barrier' submodule is initialized*/
        unsigned int wake_up_flag:1;            /**< Flag used to wake-up all thread which slept on barrier*/
    #endif

    #ifndef DISABLE_SYSFS
        unsigned int sysfs_loaded:1;            /**< 1 if the sysfs submodule is initialized correctly*/
    #endif

    unsigned int strict_mode:1; 

    unsigned int garbage_collector_disabled:1;  /**< 1 if the garbage collector is disabled, zero otherwise*/

    unsigned int gc_include_struct:1;           /**< 1 if the structure related to a message should be included in the size count */

} __attribute__((packed)) g_flags_t;


/**
 * @brief Garbage Collector structure
 * 
 * The ratio parameter indicates when the garbage collector should starts:
 *      If the current ratio is higher that the garbage collector ratio, it 
 *      perform cleaning operation, otherwise it will not start
 * 
 * Ratio is computed by dividing the current storage size with the maximum storage size_t
 * and by multiplying it by 10.
 */
typedef struct t_garbage_collector{
    struct work_struct work;        /**< Garbage Collector deferred work*/
    atomic_t ratio;                 /**< Ratio of the garbage collector*/
} garbage_collector_t;




/**
 * @brief Group device data structure
 * 
 * Contains all the private data of a group character device
 * 
 */
typedef struct group_data {
    struct cdev cdev;           /** @brief Character Device definition  */
    struct device* dev;
    dev_t deviceID;            
    int group_id;               /** @brief Unique identifier of a group. Provided by IDR */

    group_t  descriptor;        /** @brief System-wide descriptor of a group*/

    //Owner
    uid_t owner;                
    struct rw_semaphore owner_lock;

    //Members
    struct list_head active_members;            /**< List of process that opened the group*/    
    atomic_t members_count;                     /**< Number of process that opened the group*/
    struct rw_semaphore member_lock;            /**< Lock on the active members list*/

    //Message-Subsystem
    msg_manager_t *msg_manager;                 /**< Message manager subsytem*/

    //Garbage Collector
    garbage_collector_t garbage_collector;      /**< Garbage collector instance*/

    #ifndef DISABLE_THREAD_BARRIER
        //Thread-barrier
        wait_queue_head_t barrier_queue;        /**< Queue where processes which slept on a barrier are put*/
    #endif


    #ifndef DISABLE_SYSFS
        group_sysfs_t group_sysfs;              /**< Group's sysfs structure*/
    #endif


    g_flags_t flags;                            /**< Group's status flags*/
} group_data;

#endif //TYPES_H