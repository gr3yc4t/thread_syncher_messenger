#include "message.h"

//Internal Prototypes
bool isValidSizeLimits(msg_t *msg, msg_manager_t *manager);



/**
 *  @brief Print a [msg_t] (\ref msg_t) structure
 *  @param[in] msg The 'msg_t' to print
 *  @return nothing
 */
void debugMsg(msg_t msg){

    pr_debug("MESSAGE DATA");
    pr_debug("Message type size %ld", msg.size);
   pr_debug("Message author pid %d", msg.author);

    if(msg.size == sizeof(char) && msg.buffer != NULL){
        pr_debug("Message string %s", (char*)msg.buffer);
    }

}


#ifndef DISABLE_DELAYED_MSG
/**
 * @brief Check if the message delay is greater than zero on a group
 * 
 * @param[in] manager A pointer to the message manager of the group
 * 
 * @note This function is thread-safe
 * 
 * @retval true If the delay is greater than zero
 * @retval false If the delay is zero
 */
bool isDelaySet(const msg_manager_t *manager){
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
 * @param[in] timer The timer that elasped
 * @return nothing
 */
void delayedMessageCallback(struct timer_list *timer){

    struct t_message_delayed_deliver *delayed_msg;  //Elasped msg
    msg_t *msg_deliver;     //Message to add to the queue
    int ret;                           

    pr_info("delayedMessageCallback: timer elasped");


    delayed_msg = from_timer(delayed_msg, timer, delayed_timer);
    msg_deliver = &delayed_msg->message;


    if(delayed_msg->manager == NULL){
        pr_err("Message manager is not allocated, exiting");
        return;
    }

    pr_info("delayedMessageCallback: Writing message into the FIFO queue");

    if((ret = writeMessage(msg_deliver, delayed_msg->manager)) < 0){
        pr_err("delayedMessageCallback: Unable to deliver delayed message: %d", ret);
        return;
    }


    pr_debug("delayedMessage: trying to acquire lock");
    down(&delayed_msg->manager->delayed_lock);
        pr_debug("delayedMessage: lock acquired");
        list_del(&delayed_msg->delayed_list);
        kfree(delayed_msg);
    up(&delayed_msg->manager->delayed_lock);
    pr_info("delayedMessageCallback: unlocking delayed list after deleting entries");



    //Deallocate structures
    if(!del_timer(timer))
        pr_err("Strange behaviour: timer callback called but timer not elasped...");
    


    return;
}

/**
 * @brief Insert a delayed message into the pending queue
 * 
 * @param[in] message The message to insert into the queue
 * @param[in] manager A pointer to the current msg_manager_t of the group
 * 
 * 
 * @retval 0 on success
 * @retval -1 on error
 */
int queueDelayedMessage(msg_t *message, msg_manager_t *manager){
    struct t_message_delayed_deliver *newMessageDeliver;
    long delay;

    if(!message || !manager){
        pr_err("%s: NULL pointers", __FUNCTION__);
        return -1;
    }

    pr_debug("queueDelayedMessage: Checking size limits...");

    if(!isValidSizeLimits(message, manager)){
        pr_err("Message size is invalid");
        return -1;
    }

    newMessageDeliver = (struct t_message_delayed_deliver*)kmalloc(sizeof(struct t_message_delayed_deliver), GFP_KERNEL);
    if(!newMessageDeliver)
        return -1;

    newMessageDeliver->message = *message;
    newMessageDeliver->manager = manager;


    delay = atomic_long_read(&manager->message_delay);

    pr_debug("queueDelayedMessage: Delay value %ld", delay);

    timer_setup(&newMessageDeliver->delayed_timer, delayedMessageCallback, 0);

    //Add to the msg_manager message queue
    down(&manager->delayed_lock);
        //Queue Critical Section
        list_add_rcu(&newMessageDeliver->delayed_list, &manager->delayed_queue);
    up(&manager->delayed_lock);

    //Set delay and start the timer
    newMessageDeliver->delayed_timer.expires = jiffies + delay * HZ;
    add_timer(&newMessageDeliver->delayed_timer);
    pr_debug("queueDelayedMessage: Timer started");

    return 0;    
}


/**
 * @brief Remove all the delayed message from the queue
 * @param[in] manager   The message manager of the group
 * 
 * @note This function is thread safe with respect to the list of 
 *          delayed message
 * 
 * @return The number of delayed messages which revoked 
 */
int revokeDelayedMessage(msg_manager_t *manager){
    struct list_head *cursor, *temp;
    struct t_message_delayed_deliver *msgDeliver;
    int count = 0;

    pr_debug("Revoking delayed messages...");

    down(&manager->delayed_lock);

        list_for_each_safe(cursor, temp, &manager->delayed_queue){

            msgDeliver = list_entry(cursor, struct t_message_delayed_deliver, delayed_list);
            
            if(msgDeliver != NULL){
                //Stop the timer
                //TODO: check thread safety with the callback function
                pr_debug("Message timer deleted");
                del_timer(&msgDeliver->delayed_timer);
            }

            list_del_init(cursor);
            count++;
        }

        synchronize_rcu();

    up(&manager->delayed_lock);

    return count;
}


/**
 * @brief Set the delay of messages in the delay queue to zero
 * 
 * @param[in] manager The group's message manager
 * 
 * @return The number of messages which delay was cancelled
 * 
 * @note At the moment the function locks the delayed queue, set all timers to zero and 
 *      unlock the queue. This means that while the function loops through the queue,
 *      timer's callback will wait since the queue is locked.
 */
int cancelDelay(msg_manager_t *manager){
    struct list_head *cursor, *temp;
    struct t_message_delayed_deliver *msgDeliver;
    int count = 0;

    pr_debug("cancelDelay: Cancelling delay on messages...");

    down(&manager->delayed_lock);

        list_for_each_safe(cursor, temp, &manager->delayed_queue){

            msgDeliver = list_entry(cursor, struct t_message_delayed_deliver, delayed_list);
            
            if(msgDeliver != NULL){
                //Stop the timer
                //TODO: See notes. Possibly substitute with del_timer_synch
                pr_debug("cancelDelay: timer stopped");

                mod_timer(&msgDeliver->delayed_timer, 0);
                //del_timer_synch(&msgDeliver->delayed_timer);
                count++;
            }
        }

    up(&manager->delayed_lock);
    pr_debug("cancelDelay: delayed queue unlocked");

    return count;
}



#endif

/**
 * @brief Check if the structure's size should be included when computing message size
 * 
 * Message structure includes the 't_message_deliver' and 'group_members_t' 
 * 
 * @retval true if the relative flag is set
 * @retval false if the relative flag is off
 */
bool isStructSizeIncluded(msg_manager_t *manager){
    group_data *grp_data;

    grp_data = container_of(&manager, group_data, msg_manager);

    if(!grp_data)
        return false;

    pr_debug("Include struct flag value: %d", grp_data->flags.gc_include_struct);

    if(grp_data->flags.gc_include_struct == 1)
        return true;
    return false;
}



/**
 * @brief Check if the delivery of msg will exceed max sizes
 * @param[in] msg The message to test for
 * @retval true if the size limits are respected
 * @retval false if the size exceed the limits
 * 
 * @note This function is thread-safe
 */
bool isValidSizeLimits(msg_t *msg, msg_manager_t *manager){
        msg_manager_t *my_manager;
        u_long max_msg_size;
        u_long max_storage_size;
        u_long curr_storage_size;
        u_long msg_size;
        u_long structure_size;

        if(!msg || !manager)
            return false;


        msg_size = (u_long)msg->size;

        rcu_read_lock();
            my_manager = rcu_dereference(manager);

            max_msg_size = my_manager->max_message_size;
            max_storage_size = my_manager->max_storage_size;
            curr_storage_size = my_manager->curr_storage_size;
        rcu_read_unlock();

        if(msg_size > max_msg_size)
            return false;

        //Include both the uppper structure 't_message_deliver' and its recipients
        //  list that will contain the sender
        if(isStructSizeIncluded(manager))
            structure_size = sizeof(struct t_message_deliver) + sizeof(group_members_t);
        else
            structure_size = 0;
        
        pr_debug("Total new size: %lu", curr_storage_size + msg_size + structure_size);

        if(curr_storage_size + msg_size + structure_size > max_storage_size)
            return false;

        return true;
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

        pr_debug("Message participant: %d", src_elem->pid);

        if(!src_elem)
            return;

        dest_elem = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);

        if(!dest_elem)
            return;

        memcpy(dest_elem, src_elem, sizeof(group_members_t));


        list_add(&dest_elem->list, dest);
        count++;
    }


    pr_debug("Copied %d participants as message recipient", count);

}

/**
 * @brief Counts the current number of recipient in a message deliver
 * @param[in] msg_deliver A t_message_deliver structure
 * 
 * @note This function is thread-safe with respect to recipients
 * 
 * @retval The number of recipients in the list
 */
u_int count_recipients_safe(struct t_message_deliver *msg_deliver){
    struct list_head *cursor;
    u_int count = 0;

    down_read(&msg_deliver->recipient_lock);
        list_for_each(cursor, &msg_deliver->recipient){
            count++;
        }
    up_read(&msg_deliver->recipient_lock);

    return count;
}

/**
 * @brief Counts the current number of recipient in a message deliver
 * @param[in] msg_deliver A t_message_deliver structure
 * 
 * @note This function is NOT thread-safe with respect to recipients atomic_long_read
 *          should be called only when the recipient lock is held in read mode.
 * 
 * @retval The number of recipients in the list
 */
u_int count_recipients(struct t_message_deliver *msg_deliver){
    struct list_head *cursor;
    u_int count = 0;

    list_for_each(cursor, &msg_deliver->recipient){
        count++;
    }

    return count;
}

/**
 * @brief Deallocate the current recipients in a message deliver structure
 * @param[in] msg_deliver A t_message_deliver structure
 * 
 * @note This function is NOT thread-safe with respect to recipients and
 *          should be called only when the recipient lock is held in read mode. 
 * 
 * @retval The number of deallocated recipients in the list
 */
int deallocate_recipients(struct t_message_deliver *msg_deliver){
    struct list_head *cursor, *temp;
    group_members_t *elem;
    int count = 0;

    list_for_each_safe(cursor, temp, &msg_deliver->recipient){
        elem = list_entry(cursor, group_members_t, list);
        if(!elem)
            return -1;

        list_del_init(cursor);
        count++;
    }

    return count;
}

/**
 * @brief Deallocate the current recipients in a message deliver structure
 * @param[in] msg_deliver A t_message_deliver structure
 * 
 * @note This function is thread-safe with respect to recipients list
 * 
 * @retval The number of deallocated recipients in the list
 */
int deallocate_recipients_safe(struct t_message_deliver *msg_deliver){
    struct list_head *cursor, *temp;
    group_members_t *elem;
    int count = 0;

    down_write(&msg_deliver->recipient_lock);
        list_for_each_safe(cursor, temp, &msg_deliver->recipient){
            elem = list_entry(cursor, group_members_t, list);
            if(!elem)
                return -1;

            list_del_init(cursor);
            count++;
        }
    up_write(&msg_deliver->recipient_lock);
    return count;
}


/**
 * @brief Check if a given pid is present in the recepit list
 * 
 * @param[in] recipients The list of recipients of the message to check
 * @param[in] my_pid The PID to check
 * 
 * @retval true if the provided pid is present in the recipient list
 * @retval false if the pid is not present in the recipient list
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
 * @note This function is NOT thread-safe and must be called while holding a lock to 
 *          the recipient's list.
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
 * @retval true if active members is contained in recipients
 * @retval false if active members is NOT contained in recipients
 * 
 * @note This function must be called only when there is a reader lock on the active_member
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
 *  @retval 0 on success
 *  @retval -EFAULT if the copy fails
 * 
 *  @todo Check thread-safety of the function 
 *  @note   The umsg structure must be allocated
 */
__must_check int copy_msg_to_user(const msg_t *kmsg, __user char *ubuffer, const ssize_t _size){

    if(kmsg == NULL){
        pr_debug("copy_msg_to_user: kernel message NULL pointer provided");
        return -EFAULT;
    }

    if(ubuffer == NULL){
        pr_debug("copy_msg_to_user: user buffer NULL pointer provided");
        return -EFAULT;
    }

    if(!access_ok(ubuffer, _size)){
        pr_debug("copy_msg_to_user: user-space memory access is invalid");
        return -EFAULT;
    }

    if(copy_to_user(ubuffer, kmsg->buffer, _size)){
        pr_debug("copy_msg_to_user: Unable to copy msg_t structure to user");
        return -EFAULT;
    }

    return 0;
}


/**
 *  @brief copy a msg_t structure from user-space to kernel-space
 *  @param[out] kmsg Kernel-space msg_t
 *  @param[in] umsg User-space msg_t 
 *
 *  @retval 0 on success
 *  @retval -EFAULT if the copy fails
 * 
 *  @todo Check thread-safety of the function
 *  @note The kmsg structure must be allocated
 */
__must_check int copy_msg_from_user(msg_t *kmsg, const char *umsg, const ssize_t _size){
    
    void *kbuffer;

    if(kmsg == NULL || umsg == NULL)
        return -EFAULT;


    if(!access_ok(umsg, _size)){
        pr_debug("copy_msg_from_user: user-space memory access is invalid");
        return -EFAULT;        
    }

    kbuffer = (int8_t*)kmalloc(sizeof(int8_t) * _size, GFP_KERNEL);

    if(!kbuffer)
        return MEMORY_ERROR;

    // read data from user buffer to my_data->buffer 
    if (copy_from_user(kbuffer, umsg, sizeof(int8_t)*_size)){
        kfree(kbuffer);
        return -EFAULT;
    }
        

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
 * @retval An 'msg_manager_t' pointer to an allocated an initialized 'msg_manager_t' struct
 * @retval A NULL pointer in case the 'kmalloc' fails
 */
__must_check msg_manager_t *createMessageManager(const u_int _max_storage_size, const u_int _max_message_size, garbage_collector_t *garbage_collector){

    msg_manager_t *manager = (msg_manager_t*)kmalloc(sizeof(msg_manager_t), GFP_KERNEL);
    if(!manager)
        return NULL;

    manager->max_storage_size = _max_storage_size;
    manager->max_message_size = _max_message_size;
    manager->curr_storage_size = 0;

    INIT_LIST_HEAD(&manager->queue);

    init_rwsem(&manager->queue_lock);
    init_rwsem(&manager->config_lock);

    INIT_WORK(&garbage_collector->work, queueGarbageCollector);
    atomic_set(&garbage_collector->ratio, DEFAULT_GC_RATIO);

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
 * @retval 0 on success
 * @retval STORAGE_SIZE_ERR if the message does not respect the group's size limits
 * @retval ALLOC_ERR if some allocation fails
 * 
 * @note Must be protected by a write-spinlock to avoid concurrent modification of The
 *          recipient data-structure and manager's message queue
 * 
 */
int writeMessage(msg_t *message, msg_manager_t *manager){

    struct t_message_deliver *newMessageDeliver;
    group_members_t *sender;
    int ret = 0;
    u_long message_size;

    if(!isValidSizeLimits(message, manager)){
        pr_err("Message size is invalid");
        return STORAGE_SIZE_ERR;
    }

    newMessageDeliver = (struct t_message_deliver*)kmalloc(sizeof(struct t_message_deliver), GFP_KERNEL);
    if(!newMessageDeliver)
        return ALLOC_ERR;   //No need to cleanup


    newMessageDeliver->message = *message;

    init_rwsem(&newMessageDeliver->recipient_lock);

    //Set message's recipients
    INIT_LIST_HEAD(&newMessageDeliver->recipient);

    //Add the sender to the list of recipients
    sender = (group_members_t*)kmalloc(sizeof(group_members_t), GFP_KERNEL);
    
    if(!sender){
        ret = ALLOC_ERR;
        goto cleanup;
    }

    //Add the sender's PID in order to avoid reading its own messages
    sender->pid = current->pid;
    list_add_tail(&sender->list, &newMessageDeliver->recipient);


    //Add to the msg_manager message queue
    down_write(&manager->queue_lock);
        //Queue Critical Section
        list_add_tail(&newMessageDeliver->fifo_list, &manager->queue);
    up_write(&manager->queue_lock);
    pr_debug("writeMessage: queue_lock released");


    //Update storage parameters

    message_size = message->size;

    if(isStructSizeIncluded(manager))
        message_size += sizeof(struct t_message_deliver);

    pr_debug("Message Size: %lu", message_size);

    down_write(&manager->config_lock);
        manager->curr_storage_size += message_size;
    up_write(&manager->config_lock);


    return ret; 


    cleanup:
        kfree(newMessageDeliver);
        return ret;
}

/**
 * @brief Read a message from the corresponding queue
 * @retval 0 on success
 * @retval 1 if no message is present
 * @retval -1 on critical error
 */

int readMessage(msg_t *dest_buffer, msg_manager_t *manager){

    struct list_head *cursor;
    struct t_message_deliver *msg_deliver;
    pid_t pid = current->pid;


    down_read(&manager->queue_lock);
    pr_debug("readMessage: queue_lock acquired");

        //Read queue critical section
        list_for_each(cursor, &manager->queue){

            msg_deliver = NULL;
            msg_deliver = list_entry(cursor, struct t_message_deliver, fifo_list);

            if(!msg_deliver)
                goto cleanup;

            if(msg_deliver->message.author == pid){
                /**
                 * This should't be necessary since the message's sender is
                 * automatically added in the list of recipients and, consequently,
                 * the 'wasDelivered' function should skip it.
                 * However for unknown reason this does not always happens (especially
                 * if the message is delayed)
                 * 
                 * @bug: when 'revoke delay' functionality is called, messages trigger
                 *  this if and will not be delivered
                 */
                pr_debug("Message sent from the reader, skipping...");
                pr_debug("Sender PID: %d", pid);
                pr_debug("Message Content %s", (char*)msg_deliver->message.buffer);
            }else if(!wasDelivered(&msg_deliver->recipient, pid)){
                pr_info("Message found for PID: %d", (int)pid);
                //Copy the message to the destination buffer
                memcpy(dest_buffer, &msg_deliver->message, sizeof(msg_t));

                down_write(&msg_deliver->recipient_lock);
                    //Procedure to remove the current recipient from the recipient's list
                    setDelivered(&msg_deliver->recipient, pid);
                up_write(&msg_deliver->recipient_lock);

                
                up_read(&manager->queue_lock);
                pr_debug("readMessage: queue_lock released");


                //Update the current storage size with the recipient's entry in the list
                if(isStructSizeIncluded(manager)){
                    down_write(&manager->config_lock);
                        manager->curr_storage_size += sizeof(group_members_t);
                    up_write(&manager->config_lock);
                }


                return 0;
            }

            //Otherwise, continue with the next message
        }

    up_read(&manager->queue_lock);
    pr_debug("readMessage: queue_lock released");

    pr_info("No message present for PID: %d", pid);
    return 1;


    cleanup:
        up_read(&manager->queue_lock);
        pr_err("readMessage: 'list_entry' returned a NULL pointer");
        return -1;
}






/**
 * @brief Remove a message from the queue when it was completely delivered
 * @param[in] work The work struct contained inside "msg_manager_t"
 * 
 * @return nothing
 * 
 * @todo Check if size calculation are done correctly
 * 
 * @note This function has three critical section, therefore it could be a bottleneck of
 *      the module
 */
void queueGarbageCollector(struct work_struct *work){
    group_data *grp_data;
    garbage_collector_t *garbage_collector;
    msg_manager_t *manager;
    
    struct list_head *current_member;
    struct list_head *cursor;
    struct list_head *temp;
    
    unsigned int deleted_entries;
    unsigned int deleted_recipients;
    int del_recipients;

    u_long deleted_deliver_size;
    u_long total_recipients_size;
    u_long total_msg_size;
    u_long total_deleted_size;




    garbage_collector = container_of(work, garbage_collector_t, work);
    if(!garbage_collector)
        return;

    grp_data = container_of(garbage_collector, group_data, garbage_collector);
    if(!grp_data)
        return;


    pr_debug("Garbage Collector starting...");

    deleted_entries = 0;
    deleted_recipients = 0;
    deleted_deliver_size = 0;
    total_msg_size = 0;
    total_recipients_size = 0;
    total_deleted_size = 0;

    down_read(&grp_data->member_lock);

        current_member = &grp_data->active_members;

        if(!down_write_trylock(&grp_data->msg_manager->queue_lock)){
            pr_debug("Garbage Collector: Unable to acquire queue lock, skipping...");
            up_read(&grp_data->member_lock);
            return;
        }
        //Queue Critical Section

            list_for_each_safe(cursor, temp, &grp_data->msg_manager->queue){

                struct t_message_deliver *entry = list_entry(cursor, struct t_message_deliver, fifo_list);

                down_read(&entry->recipient_lock);
                //Recipient critical section

                    if(isDeliveryCompleted(&entry->recipient, current_member)){
                        pr_debug("Garbage Collector: deleting entry from queue");

                        kfree(entry->message.buffer);  //Message Buffer
                        total_msg_size += entry->message.size;

                        if( (del_recipients = deallocate_recipients(entry)) > 0)
                            deleted_recipients += del_recipients;
                        
                        list_del_init(cursor);  //TODO: check if the recipients list is deallocated

                        kfree(entry);   //t_message_deliver 

                        deleted_entries++;
                    }

                up_read(&entry->recipient_lock);                
            }

        up_write(&grp_data->msg_manager->queue_lock);

    up_read(&grp_data->member_lock);


    //Update storage parameters
    manager = grp_data->msg_manager;
    if(isStructSizeIncluded(manager)){
        deleted_deliver_size = deleted_entries * sizeof(struct t_message_deliver);
        total_recipients_size = deleted_recipients * sizeof(group_members_t);
    }

    total_deleted_size = total_msg_size + deleted_deliver_size + total_recipients_size;

    down_write(&manager->config_lock);
        if(total_deleted_size > manager->curr_storage_size)
            manager->curr_storage_size = 0;
        else
            manager->curr_storage_size -= total_deleted_size;
    up_write(&manager->config_lock);

}
