/**
 * @file		message.c
 * @brief		Handles all procedures releated to messages releated to a group device
 *
 */
#include "message.h"

//Internal Prototypes
bool isValidSizeLimits(msg_t *msg, msg_manager_t *manager);



/**
 *  @brief Print a [msg_t] (\ref msg_t) structure
 *  @param[in] msg The 'msg_t' to print
 *  @return nothign
 */
void debugMsg(msg_t msg){

    printk(KERN_DEBUG "MESSAGE DATA");
    printk(KERN_DEBUG "Message type size %ld", msg.size);
    printk(KERN_DEBUG "Message author pid %d", msg.author);

    if(msg.size == sizeof(char) && msg.buffer != NULL){
        printk(KERN_DEBUG "Message string %s", (char*)msg.buffer);
    }

}


#ifndef DISABLE_DELAYED_MSG
bool isDelaySet(const msg_manager_t *manager){
    printk(KERN_DEBUG "Reading message delay data");
    if(atomic_long_read(&manager->message_delay) != 0)
        return true;
    else
        return false;
    
}

/**
 * @brief Called when a timer for a delayed message expires
 * 
 * The function simply take the existing 't_message_delayed_deliver' structure,
 * extract the 'msg_t' field and write it into the FIFO queue via the 'writeMessage'
 * function.
 * 
 */
void delayedMessageCallback(struct timer_list *timer){

    struct t_message_delayed_deliver *delayed_msg;  //Elasped msg
    msg_t *msg_deliver;          //Message to add to the queue

    printk(KERN_INFO "delayedMessageCallback: timer elasped");


    delayed_msg = from_timer(delayed_msg, timer, delayed_timer);

    msg_deliver = &delayed_msg->message;


    if(delayed_msg->manager == NULL){
        printk(KERN_ERR "Message manager is not allocated, exiting");
        return;
    }

    printk(KERN_INFO "delayedMessageCallback: Writing message into the FIFO queue");

    if(!writeMessage(msg_deliver, delayed_msg->manager)){
        printk(KERN_ERR "Unable to deliver delayed message");
        return;
    }

    //Deallocate structures
    printk(KERN_DEBUG "delayedMessageCallback: start deallocating structures");

    if(del_timer(timer)){
        printk(KERN_DEBUG "Strange behaviour: timer callback called but timer not elasped...");
    }

    printk(KERN_INFO "delayedMessageCallback: locking delayed list");
    down(&delayed_msg->manager->delayed_lock);
        list_del(&delayed_msg->delayed_list);
        kfree(delayed_msg);
    up(&delayed_msg->manager->delayed_lock);
    printk(KERN_INFO "delayedMessageCallback: unlocking delayed list");


    return;
}

/**
 * @brief Insert a delayed message into the pending queue
 * 
 * @param[in] message The message to insert into the queue
 * @param[in] manager A pointer to the current msg_manager_t of the group
 * 
 * 
 * @return 0 on success, -1 on error
 */
int queueDelayedMessage(msg_t *message, const msg_manager_t *manager){
    struct t_message_delayed_deliver *newMessageDeliver;
    group_members_t *sender;


    if(!message || !manager){
        printk(KERN_ERR "%s: NULL pointers", __FUNCTION__);
        BUG();
    }


    printk(KERN_DEBUG "queueDelayedMessage: Queuing a delayed message...");

    pid_t pid = current->pid;

    debugMsg(*message);

    printk(KERN_DEBUG "queueDelayedMessage: Checking size limits...");

    if(!isValidSizeLimits(message, manager)){
        printk(KERN_ERR "Message size is invalid");
        return -1;
    }

    newMessageDeliver = (struct t_message_delayed_deliver*)kmalloc(sizeof(struct t_message_delayed_deliver), GFP_KERNEL);
    if(!newMessageDeliver)
        return -1;

    newMessageDeliver->message = *message;
    newMessageDeliver->manager = manager;

    printk(KERN_DEBUG "queueDelayedMessage: reading delay...");


    long delay = atomic_long_read(&manager->message_delay);

    printk(KERN_DEBUG "queueDelayedMessage: Delay value %ld", delay);

    printk(KERN_DEBUG "queueDelayedMessage: Setting up delayed timer...");
    timer_setup(&newMessageDeliver->delayed_timer, delayedMessageCallback, 0);

    printk(KERN_DEBUG "queueDelayedMessage: Trying to acquire delayed_lock ");

    //Add to the msg_manager message queue
    down(&manager->delayed_lock);
    printk(KERN_DEBUG "queueDelayedMessage: delayed_lock acquired");
        //Queue Critical Section
        list_add(&newMessageDeliver->delayed_list, &manager->delayed_queue);
    up(&manager->delayed_lock);
    printk(KERN_DEBUG "queueDelayedMessage: delayed_lock released");

    //Set delay and start the timer
    newMessageDeliver->delayed_timer.expires = jiffies + delay * HZ;
    add_timer(&newMessageDeliver->delayed_timer);
    printk(KERN_DEBUG "queueDelayedMessage: Timer started");

    return 0;    

}


/**
 * 
 * @return The number of delayed messages which revoked 
 * 
 */
int revokeDelayedMessage(msg_manager_t *manager){

    printk(KERN_DEBUG "Revoking delayed messages...");

    struct list_head *cursor, *temp;
    struct t_message_delayed_deliver *msgDeliver;
    int count = 0;

    down(&manager->delayed_lock);

        list_for_each_safe(cursor, temp, &manager->delayed_queue){

            msgDeliver = list_entry(cursor, struct t_message_delayed_deliver, delayed_list);
            
            if(msgDeliver != NULL){
                //Stop the timer
                //TODO: check thread safety with the callback function
                del_timer(&msgDeliver->delayed_timer);
            }

            list_del_init(cursor);

            count++;
        }

    up(&manager->delayed_lock);

    return count;
}


/**
 * 
 * @return The number of messages which delay was cancelled
 * 
 * @note At the moment the function locks the delayed queue, set all timers to zero and 
 *      unlock the queue. This means that while the function loops through the queue,
 *      timer's callback will wait since the queue is locked.
 */
int cancelDelay(msg_manager_t *manager){

    printk(KERN_DEBUG "cancelDelay: Cancelling delay on messages...");

    struct list_head *cursor, *temp;
    struct t_message_delayed_deliver *msgDeliver;
    int count = 0;

    printk(KERN_DEBUG "cancelDelay: trying to acquire delayed_lock");
    down(&manager->delayed_lock);
        printk(KERN_DEBUG "cancelDelay: delayed_lock acquired");

        list_for_each_safe(cursor, temp, &manager->delayed_queue){

            msgDeliver = list_entry(cursor, struct t_message_delayed_deliver, delayed_list);
            
            if(msgDeliver != NULL){
                //Stop the timer
                //TODO: See notes. Possibly substitute with del_timer_synch
                printk(KERN_DEBUG "cancelDelay: timer stopped");

                mod_timer(&msgDeliver->delayed_timer, 0);
                count++;
            }
        }

    up(&manager->delayed_lock);
    printk(KERN_DEBUG "cancelDelay: delayed_lock released");

    return count;

}



#endif


/**
 * @brief Check if the delivery of msg will exceed max sizes
 * @param[in] msg The message to test for
 * @return true if the size limits are respected, false otherwise
 * 
 * @note This function is thread-safe
 */
bool isValidSizeLimits(msg_t *msg, msg_manager_t *manager){
        u_int msg_size;

        if(!msg || !manager){
            printk(KERN_ERR "%s: NULL pointers", __FUNCTION__);
            BUG();
        }

        printk(KERN_DEBUG "READ-LOCK: config_lock");
        down_read(&manager->config_lock);
            msg_size = msg->size;

            printk(KERN_DEBUG "Message size: %d", msg_size);

            if(msg_size > manager->max_message_size){
                printk(KERN_DEBUG "Max msg. size invalidated!!!");
                goto invalid;
            }

            if(manager->curr_storage_size + msg_size > manager->max_storage_size){
                printk(KERN_DEBUG "Max msg. storage size invalidated!!!");
                goto invalid;
            }

        up_read(&manager->config_lock);
        printk(KERN_DEBUG "READ-UNLOCK: config_lock");

        return true;

    invalid:
        up_read(&manager->config_lock);
        return false;
}


/**
 * @brief copy the list of participants into a new list
 */
void copy_current_participants(struct list_head *dest, struct list_head *source){
    struct list_head *cursor;

    int count = 0;  //TODO: Remove after finished debugging or use @ifdef

    list_for_each(cursor, source){

        group_members_t *dest_elem;
        group_members_t *src_elem = NULL;

        src_elem = list_entry(cursor, group_members_t, list);

        printk(KERN_INFO "Message participant: %d", src_elem->pid);

        if(!src_elem)
            return;

        dest_elem = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);

        if(!dest_elem)
            return;

        memcpy(dest_elem, src_elem, sizeof(group_members_t));


        list_add(&dest_elem->list, dest);
        count++;
    }


    printk(KERN_DEBUG "Copied %d participants as message recipient", count);

}

/**
 * @brief Check if a given pid is present in the recepit list
 * 
 * @param[in] recipients The list of recipients of the message to check
 * @param[in] my_pid The PID to check
 * 
 * @return true if the provided pid is present in the recipient list, false otherwise
 * 
 * @note This function is not thread-safe, so it must be called after locking the recipient list
 * @note A possible improvement would sort the list and return if the current member's
 *          pid is greater than 'my_pid'
 */
bool wasDelivered(const struct list_head *recipients, const pid_t my_pid){
    
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


/**
 * @brief Add a PID to the delivered list of a message
 * @param[in] recipients The list_head structure of a 't_message_deliver' struct
 * @param[in] my_pid     The pid to add
 * 
 * @return nothing
 */
void setDelivered(struct list_head *recipients, const pid_t my_pid){

    group_members_t *member = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);
    if(!member)
        return;

    member->pid = my_pid;
    list_add_tail(&member->list, recipients);

    return;
}



/**
 * @brief Checks if the set of recipients of a message contains all the active members
 * @param[in] recipients The recipients of the message to 'wasDelivered'
 * @param[in] active_member The list of active members to compare
 * 
 * @return true if active members is contained in recipients, false otherwise
 * 
 * @note This function must be called only when there is a reader lock on both active_member
 *      recipients structure
 */
bool isDeliveryCompleted(const struct list_head *recipients, const struct list_head *active_member){

    struct list_head *cursor;
    group_members_t *member;
    pid_t member_pid;

    list_for_each(cursor, active_member){

        member = list_entry(cursor, group_members_t, list);
        member_pid = member->pid;
        
        if(!wasDelivered(recipients, member_pid)){
            return false;
        }

    }

    return true;
}




/**
 *  @brief copy a msg_t structure from kernel-space to user-space 
 *  @param[in] kmsg Kernel-space msg_t
 *  @param[out] umsg User-space buffer 
 *
 *  @return 0 on success, EFAULT if copy fails
 *  @todo Check thread-safety of the function 
 *  @note   The umsg structure must be allocated
 */
int copy_msg_to_user(const msg_t *kmsg, __user int8_t *ubuffer, const ssize_t _size){

    if(kmsg == NULL || ubuffer == NULL){
        printk(KERN_DEBUG "copy_msg_to_user: NULL pointer provided");
        return -EFAULT;
    }

    if(copy_to_user(ubuffer, kmsg->buffer, _size)){
        printk(KERN_DEBUG "copy_msg_to_user: Unable to copy msg_t structure to user");
        return -EFAULT;
    }

    return 0;
}


/**
 *  @brief copy a msg_t structure from user-space to kernel-space
 *  @param[out] kmsg Kernel-space msg_t
 *  @param[in] umsg User-space msg_t 
 *
 *  @return 0 on success, EFAULT if copy fails
 *  @todo Check thread-safety of the function
 *  @note The kmsg structure must be allocated
 */
int copy_msg_from_user(msg_t *kmsg, const int8_t *umsg, const ssize_t _size){
    
    void *kbuffer;

    if(kmsg == NULL || umsg == NULL)
        return -EFAULT;

    kbuffer = (int8_t*)kmalloc(sizeof(int8_t) * _size, GFP_KERNEL);

    if(!kbuffer)
        return MEMORY_ERROR;

    // read data from user buffer to my_data->buffer 
    if (copy_from_user(kbuffer, umsg, sizeof(int8_t)*_size))
        return -EFAULT;

    kmsg->size = _size;
    kmsg->buffer = kbuffer;

    return 0;
}


/**
 * @brief Allocate and initialize all the members of a 'msg_manager_t' struct
 * @param[in] _max_message_size    Configurable param
 * @param[in] _max_storage_size    Configurable param
 * @param[in] garbageCollectorFunction Pointer to the work_struct responsible for garbage collection 
 * 
 * @return msg_manager_t    A pointer to an allocated an initialized 'msg_manager_t' struct
 */
msg_manager_t *createMessageManager(u_int _max_storage_size, u_int _max_message_size, struct work_struct *garbageCollector){

    msg_manager_t *manager = (msg_manager_t*)kmalloc(sizeof(msg_manager_t), GFP_KERNEL);
    if(!manager)
        return NULL;

    manager->max_storage_size = _max_storage_size;
    manager->max_message_size = _max_message_size;
    manager->curr_storage_size = 0;

    INIT_LIST_HEAD(&manager->queue);

    init_rwsem(&manager->queue_lock);
    init_rwsem(&manager->config_lock);

    INIT_WORK(garbageCollector, queueGarbageCollector);

    #ifndef DISABLE_DELAYED_MSG
        sema_init( &manager->delayed_lock, 1);
        atomic_long_set(&manager->message_delay, 0);
        INIT_LIST_HEAD(&manager->delayed_queue);
    #endif

    return manager;
}


/**
 * @brief write message on a group queue
 * @param[in] message   The data pointed must never be deallocatated
 * @param[in] manager   Pointer to the message manager
 * @param[in] recipients    The head of the linked list containing the thread recipients
 * 
 * @return 0 on success, negative number otherwise
 * 
 * @note Must be protected by a write-spinlock to avoid concurrent modification of The
 *          recipient data-structure and manager's message queue
 * 
 * @todo Define clear error code
 */
int writeMessage(msg_t *message, msg_manager_t *manager){

    struct t_message_deliver *newMessageDeliver;
    group_members_t *sender;

    pid_t pid = current->pid;

    debugMsg(*message);

    if(!isValidSizeLimits(message, manager)){
        printk(KERN_ERR "Message size is invalid");
        return -1;
    }    //Set message's recipients



    newMessageDeliver = (struct t_message_deliver*)kmalloc(sizeof(struct t_message_deliver), GFP_KERNEL);
    if(!newMessageDeliver)
        return -1;

    BUG_ON(message == NULL);

    newMessageDeliver->message = *message;

    init_rwsem(&newMessageDeliver->recipient_lock);

    //Set message's recipients
    INIT_LIST_HEAD(&newMessageDeliver->recipient);

    //Add the sender to the list of recipients
    sender = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);
    
    if(!sender) //TODO: handle better
        return -1;

    sender->pid = current->pid;
    list_add_tail(&sender->list, &newMessageDeliver->recipient);


    //Add to the msg_manager message queue
    down_write(&manager->queue_lock);
    printk(KERN_DEBUG "writeMessage: queue_lock acquired");
        //Queue Critical Section
        list_add_tail(&newMessageDeliver->fifo_list, &manager->queue);
    up_write(&manager->queue_lock);
    printk(KERN_DEBUG "writeMessage: queue_lock released");

    return 0;
}

/**
 * @brief Read a message from the corresponding queue
 * @return 0 on success, -1 if no message is present
 */

int readMessage(msg_t *dest_buffer, msg_manager_t *manager){

    pid_t pid = current->pid;

    struct list_head *cursor;
    struct t_message_deliver *msg_deliver;
    
    down_read(&manager->queue_lock);
    printk(KERN_DEBUG "readMessage: queue_lock acquired");

        //Read queue critical section

        list_for_each(cursor, &manager->queue){

            msg_deliver = NULL;

            msg_deliver = list_entry(cursor, struct t_message_deliver, fifo_list);

            if(!wasDelivered(&msg_deliver->recipient, pid)){
                printk(KERN_INFO "Message found for PID: %d", (int)pid);
                //Copy the message to the destination buffer
                memcpy(dest_buffer, &msg_deliver->message, sizeof(msg_t));

                down_write(&msg_deliver->recipient_lock);
                    //Procedure to remove the current recipient from the recipient's list
                    setDelivered(&msg_deliver->recipient, pid);
                up_write(&msg_deliver->recipient_lock);

                
                up_read(&manager->queue_lock);
                printk(KERN_DEBUG "readMessage: queue_lock released");

                return 0;
            }

            //Otherwise, continue with the next message
        }

    up_read(&manager->queue_lock);
    printk(KERN_DEBUG "readMessage: queue_lock released");

    printk(KERN_INFO "No message present for PID: %d", pid);
    return 1;
}






/**
 * @brief Remove a message from the queue when it was completely delivered
 * @param [in] work The work struct contained inside "msg_manager_t"
 * 
 * @return nothing
 * 
 * @note This function has three critical section, therefore it could be a bottleneck of
 *      the module
 */
void queueGarbageCollector(struct work_struct *work){
    
    group_data *grp_data;
    grp_data = container_of(work, group_data, garbage_collector_work);

    printk(KERN_DEBUG "Garbage Collector starting...");

    down_read(&grp_data->member_lock);
    
        //printk(KERN_DEBUG "Garbage Collector: active members read locked");


        struct list_head *current_member = &grp_data->active_members;

        struct list_head *cursor;
        struct list_head *temp;

        if(!down_write_trylock(&grp_data->msg_manager->queue_lock)){
            printk(KERN_DEBUG "Garbage Collector: Unable to acquire queue lock, skipping...");
            up_read(&grp_data->member_lock);
            return;
        }
        //Queue Critical Section

            //printk(KERN_DEBUG "Garbage Collector: msg_queue write locked");

            list_for_each_safe(cursor, temp, &grp_data->msg_manager->queue){

                struct t_message_deliver *entry = list_entry(cursor, struct t_message_deliver, fifo_list);

                down_read(&entry->recipient_lock);
                //Recipient critical section

                    //printk(KERN_DEBUG "Garbage Collector: recipient read locked");

                    if(isDeliveryCompleted(&entry->recipient, current_member)){
                        printk(KERN_DEBUG "Garbage Collector: deleting entry from queue");

                        kfree(entry->message.buffer);  //Message Buffer

                        list_del_init(cursor);

                        kfree(entry);   //t_message_deliver 
                    }

                up_read(&entry->recipient_lock);                
                
                //printk(KERN_DEBUG "Garbage Collector: recipient read unlocked");
            }

        up_write(&grp_data->msg_manager->queue_lock);

        //printk(KERN_DEBUG "Garbage Collector: msg_queue write unlocked");

    up_read(&grp_data->member_lock);

    //printk(KERN_DEBUG "Garbage Collector: active members read unlocked");
}
