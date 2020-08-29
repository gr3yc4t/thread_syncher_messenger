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

typedef struct t_message{
    pid_t author;   //Author thread

    void *buffer;

    ssize_t size;
    ssize_t count;
} msg_t;


/** @brief Threads that are recipient the group device */
struct participants{
    pid_t pid;
    struct list_head list;
}


struct t_message_deliver{
    msg_t message;
    struct participants recipient; //PIDs of threads that should read 'message'

    struct list_head fifo_list;
};





typedef struct t_message_manager{
    u_int max_message_size;
    u_int max_storage_size;

    struct t_message_deliver queue;

} msg_manager_t;






inline pid_t getPID(){
    return current->pid;
}


msg_manager_t *createMessageManager(u_int _max_storage_size, u_int _max_message_size);


int writeMessage(msg_t *message, ssize_t size, ssize_t count, group_data *grp_data);
int readMessage(msg_t *dest_buffer, ssize_t size, ssize_t count, group_data *grp_data);


#endif //MSG_H