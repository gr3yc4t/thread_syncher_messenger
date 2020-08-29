#include "message.h"



void copy_current_participants(struct participants *source, struct participants *dest){
    struct participants *cursor;

    int count = 0;  //TODO: Remove after finished debugginh

    list_for_each(cursor, source){
        list_add(&dest->list, &cursor->list);
        count++;
    }


    pr_debug("Copied %d participants as message recipient", count);

}








msg_manager_t *createMessageManager(u_int _max_storage_size, u_int _max_message_size){

    msg_manager_t *manager = (msg_manager_t*)kmalloc(sizeof(msg_manager_t), GFP_KERNEL);
    if(!manager)
        return NULL;

    manager->max_storage_size = _max_storage_size;
    manager->max_message_size = _max_message_size;

    INIT_LIST_HEAD(&manager->queue->fifo_list);
    INIT_LIST_HEAD(&manager->queue->recipient->list);

    return manager;
}



int writeMessage(msg_t *message, ssize_t size, ssize_t count, group_data *grp_data){
/*
    msg_manager_t manager = grp_data->msg_manager;

    pid_t pid = current->pid;

    struct t_message_deliver newMessage;

    newMessage.message = *message;
*/



}