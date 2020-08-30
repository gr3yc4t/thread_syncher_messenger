#include "message.h"



void copy_current_participants(struct list_head *dest, struct list_head *source){
    struct list_head *cursor;

    int count = 0;  //TODO: Remove after finished debugging

    list_for_each(cursor, source){

        group_members_t *src_elem = NULL;

        src_elem = list_entry(cursor, group_members_t, list);

        printk(KERN_INFO "Message participant: %d", src_elem->pid);

        if(!src_elem)
            return;


        group_members_t *dest_elem = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);

        if(!dest_elem)
            return;

        memcpy(dest_elem, src_elem, sizeof(group_members_t));


        list_add(&dest_elem->list, dest);
        count++;
    }


    printk(KERN_INFO "Copied %d participants as message recipient", count);

}

/**
 * @brief Check if a given pid is present in the recepit list
 * 
 * 
 * @note A possible improvement would sort the list and return if the current member's
 *          pid is greater than 'my_pid'
 */
bool checkRecepit(struct list_head *recipients, const pid_t my_pid){
    
    struct list_head *cursor;

    list_for_each(cursor, recipients){

        group_members_t *member = NULL;

        member = list_entry(cursor, group_members_t, list);

        if(member->pid == my_pid){
            return true;
        }
    }

    return false;

}




int copy_msg_to_user(msg_t *kmsg, msg_t *umsg){

}

int copy_msg_from_user(msg_t *kmsg, msg_t *umsg){
    
    // read data from user buffer to my_data->buffer 
    if (copy_from_user(kmsg, umsg, sizeof(msg_t)))
        return -EFAULT;


    //Copy the 'buffer' filed of 'msg_t' into kernel space
    ssize_t buffer_size = kmsg->size;
    ssize_t buffer_elem = kmsg->count;
    ssize_t total_size = buffer_size * buffer_elem;

    printk(KERN_DEBUG "Buffer size: %ld", total_size);

    void *kbuffer = kmalloc( buffer_size * buffer_elem,  GFP_KERNEL);

    if (copy_from_user(kbuffer, umsg->buffer, total_size))
        return -EFAULT;

    kmsg->buffer = kbuffer;  //Switch pointers

    return 0;
}



msg_manager_t *createMessageManager(u_int _max_storage_size, u_int _max_message_size){

    msg_manager_t *manager = (msg_manager_t*)kmalloc(sizeof(msg_manager_t), GFP_KERNEL);
    if(!manager)
        return NULL;

    manager->max_storage_size = _max_storage_size;
    manager->max_message_size = _max_message_size;

    INIT_LIST_HEAD(&manager->queue);

    return manager;
}


/**
 * @brief write message on a group queue
 * @param message   The data pointed must never be deallocatated
 * @param size      The type of data (sizeof(type))
 * @param count     The number of 'size' bytes to read
 * @param manager   Pointer to the message manager
 * @param recipients    The head of the linked list containing the thread recipients
 * 
 * @return 0 on success, negative number otherwise
 * 
 * @note Must be protected by a write-spinlock to avoid concurrent modification of The
 *          recipient data-structure and manager's message queue
 */
int writeMessage(msg_t *message, msg_manager_t *manager, struct list_head *recipients){

    pid_t pid = current->pid;

    struct t_message_deliver *newMessageDeliver;

    newMessageDeliver = (struct t_message_deliver*)kmalloc(sizeof(struct t_message_deliver), GFP_KERNEL);
    if(!newMessageDeliver)
        return -1;

    BUG_ON(message == NULL);

    newMessageDeliver->message = *message;

    //Set message's recipients
    INIT_LIST_HEAD(&newMessageDeliver->recipient);
    copy_current_participants(&newMessageDeliver->recipient, recipients);

    //TODO: remove the sender from the recipient list

    //Add to the msg_manager message queue
    list_add_tail(&newMessageDeliver->fifo_list, &manager->queue);

}

/**
 * @brief Read a message from the corresponding queue
 * @return 0 on success, -1 if no message is present
 * @todo Implement safe concurrent access
 */

int readMessage(msg_t *dest_buffer, msg_manager_t *manager){

    pid_t pid = current->pid;

    struct list_head *cursor;

    list_for_each(cursor, &manager->queue){

        struct t_message_deliver *p = NULL;

        p = list_entry(cursor, struct t_message_deliver, fifo_list);

        if(checkRecepit(&p->recipient, pid)){
            printk(KERN_INFO "Message found for PID: %d", (int)pid);
            //Copy the message to the destination buffer
            memcpy(dest_buffer, &p->message, sizeof(msg_t));
            return 0;
        }

        //Otherwise, continue with the next message
    }


    printk(KERN_INFO "No message present for PID: %d", pid);
    return -1;
}