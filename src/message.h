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
#include <linux/kfifo.h>
#include <linux/spinlock.h>

#include <linux/rwsem.h>

typedef struct t_message{
    pid_t author;   //Author thread

    void *buffer;

    ssize_t size;
    ssize_t count;
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

    struct list_head queue;
    struct rw_semaphore queue_lock;
} msg_manager_t;




msg_manager_t *createMessageManager(u_int _max_storage_size, u_int _max_message_size);


int writeMessage(msg_t *message, msg_manager_t *manager, struct list_head *recipients);
int readMessage(msg_t *dest_buffer, msg_manager_t *manager);

int copy_msg_to_user(const msg_t *kmsg, msg_t *umsg);
int copy_msg_from_user(msg_t *kmsg, const msg_t *umsg);


#endif //MSG_H