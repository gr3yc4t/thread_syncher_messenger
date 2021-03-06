/**
 * @file		message.h
 * @brief		Handles all procedures releated to messages of a group device
 *
 */

#ifndef MSG_H
#define MSG_H

#include <linux/init.h>
#include <linux/module.h>	/* MODULE_*, module_* */
#include <linux/kernel.h>	/* We're doing kernel work */
#include <linux/slab.h>		/* kmalloc(), kfree() */
#include <linux/uaccess.h>	/* copy_from_user(), copy_to_user() */
#include <linux/semaphore.h>	/* used acces to semaphore, process management syncronization behaviour */
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>


#include "types.h"


#define NO_MSG_PRESENT          0
#define ALLOC_ERR 			    -2	 
#define USER_COPY_ERR		    -3
#define SEM_INIT_ERR            -4  /** @brief Semaphore initialization **/
#define MSG_INVALID_FORMAT      -11
#define MSG_SIZE_ERROR          -12
#define MEMORY_ERROR            -13
#define STORAGE_SIZE_ERR        -14
#define ALLOC_ERR 		        -2
#define CLASS_EXISTS		    -10
#define CLASS_ERR		        -11
#define DEV_CREATION_ERR        -12
#define CDEV_ALLOC_ERR		    -13
#define EMPTY_LIST              -20
#define NODE_NOT_FOUND          -21
#define INVALID_IOCTL_COMMAND   -1
#define CHDEV_ALLOC_ERR         -22



#define DEFAULT_GC_RATIO 3





msg_manager_t *createMessageManager(const u_int _max_storage_size, const u_int _max_message_size, garbage_collector_t *garbageCollector);

int writeMessage(msg_t *message, msg_manager_t *manager);
int readMessage(msg_t *dest_buffer, msg_manager_t *manager);

int copy_msg_from_user(msg_t *kmsg, const char *umsg, const ssize_t _size);
int copy_msg_to_user(const msg_t *kmsg, __user char *ubuffer, const ssize_t _size);

void queueGarbageCollector(struct work_struct *work);


#ifndef DISABLE_DELAYED_MSG
    bool isDelaySet(const msg_manager_t *manager);
    void delayedMessageCallback(struct timer_list *timer);
    int queueDelayedMessage(msg_t *message, msg_manager_t *manager);
    int revokeDelayedMessage(msg_manager_t *manager);
    int cancelDelay(msg_manager_t *manager);
#endif


#endif //MSG_H