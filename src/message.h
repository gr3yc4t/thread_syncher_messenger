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

#include "types.h"


#define NO_MSG_PRESENT 0
#define MSG_INVALID_FORMAT -11
#define MSG_SIZE_ERROR  -12
#define MEMORY_ERROR -13







msg_manager_t *createMessageManager(u_int _max_storage_size, u_int _max_message_size, struct work_struct *garbageCollector);

int writeMessage(msg_t *message, msg_manager_t *manager, struct list_head *recipients);
int readMessage(msg_t *dest_buffer, msg_manager_t *manager);

int copy_msg_from_user(msg_t *kmsg, const int8_t *umsg, const ssize_t _size);
int copy_msg_to_user(const msg_t *kmsg, __user int8_t *ubuffer, const ssize_t _size);

void queueGarbageCollector(struct work_struct *work);
#endif //MSG_H