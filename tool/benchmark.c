#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#include "thread_synch.h"
#include "inih/ini.h"

thread_synch_t *main_syncher;
thread_group_t *curr_group = NULL;

pthread_rwlock_t lock_rw = PTHREAD_RWLOCK_INITIALIZER;

typedef struct{
    char *group_name;
    char *group_operation;
    int group_id;
    long delay_value;
    long max_message_size;
    long max_storage_size;
    long garbage_collector_ratio;
    bool strict_mode;
    bool include_struct_size;
} configuration;




static int handler(void* user, const char* section, const char* name, const char* value){
    configuration* pconfig = (configuration*)user;

    group_t descriptor;
    int group_id;
    int ret;
    uid_t owner;
    char *buffer;
    size_t len;
    thread_group_t *tmp;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("group", "install")) {
        descriptor.group_name = strdup(value);
        descriptor.name_len = strlen(descriptor.group_name);


        tmp = installGroup(descriptor, main_syncher);
        if(tmp != NULL){
            pthread_rwlock_wrlock(&lock_rw);
                free(curr_group);
                curr_group = tmp;
            pthread_rwlock_unlock(&lock_rw);
        }

        free(descriptor.group_name);
    } else if (MATCH("group", "loadDescriptor")) {
        descriptor.group_name = strdup(value);
        descriptor.name_len = strlen(descriptor.group_name);

        tmp = loadGroupFromDescriptor(&descriptor, main_syncher);
        if(tmp != NULL){
            pthread_rwlock_wrlock(&lock_rw);
                free(curr_group);
                curr_group = tmp;
            pthread_rwlock_unlock(&lock_rw);
        }
        free(descriptor.group_name);
    } else if (MATCH("group", "loadID")) {
        group_id = atoi(value);

        tmp = loadGroupFromID(group_id);
        if(tmp != NULL){
            pthread_rwlock_wrlock(&lock_rw);
                free(curr_group);
                curr_group = tmp;
            pthread_rwlock_unlock(&lock_rw);
        }
    } else if (MATCH("message", "write")) {
        
        buffer = strdup(value);
        len = strlen(buffer);

        //printf("\n[D]Writing %s", pconfig->message);
        pthread_rwlock_rdlock(&lock_rw);
            if(openGroup(curr_group) < 0){
                pthread_rwlock_unlock(&lock_rw);
                free(buffer);
                return -1;
            }
            writeMessage(buffer, len, curr_group);
        pthread_rwlock_unlock(&lock_rw);
        free(buffer);
    } else if (MATCH("message", "read")) {
        len = atoi(value);

        //printf("\n[D]Reading %d", pconfig->len);
        buffer = (char*)malloc(sizeof(char)*(len+1));
        if(!buffer)
            return -1;


        pthread_rwlock_rdlock(&lock_rw);
            if(openGroup(curr_group) < 0){
                pthread_rwlock_unlock(&lock_rw);
                free(buffer);
                return -1;
            }
            readMessage(buffer, len, curr_group);
        pthread_rwlock_unlock(&lock_rw);
        free(buffer);
    } else if (MATCH("message", "delay")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        
        pconfig->delay_value = atol(value);        
        setDelay(pconfig->delay_value, curr_group);
    } else if (MATCH("message", "revoke_delay")) {
        if(openGroup(curr_group) < 0)
            return -1;
        revokeDelay(curr_group);
    } else if (MATCH("message", "flush")) {
        if(openGroup(curr_group) < 0)
            return -1;
        flushDelayedMsg(curr_group);
    } else if (MATCH("synch", "sleep")) {
        if(openGroup(curr_group) < 0)
            return -1;

        pconfig->delay_value = atol(value);
        sleepOnBarrier(curr_group);
    } else if (MATCH("synch", "awake")) {
        if(openGroup(curr_group) < 0)
            return -1;

        pconfig->delay_value = atol(value);
        awakeBarrier(curr_group);
    } else if (MATCH("config", "max_message_size")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        
        pconfig->max_message_size = atol(value);
        setMaxMessageSize(curr_group, pconfig->max_message_size);
    } else if (MATCH("config", "max_storage_size")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        pconfig->max_storage_size = atol(value);
        setMaxStorageSize(curr_group, pconfig->max_storage_size);
    } else if (MATCH("config", "garbage_collector_ratio")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        
        pconfig->garbage_collector_ratio = atol(value);
        setGarbageCollectorRatio(curr_group, pconfig->garbage_collector_ratio);
    } else if (MATCH("config", "include_structure_size")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        pconfig->include_struct_size = atoi(value);
        includeStructureSize(curr_group, pconfig->max_storage_size);
    } else if (MATCH("config", "read_test")) {
        if(openGroup(curr_group) < 0)
            return -1;
        getMaxMessageSize(curr_group);
        getMaxStorageSize(curr_group);
        getCurrentStorageSize(curr_group);
    } else if (MATCH("security", "strict_mode")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        
        pconfig->strict_mode = atoi(value);
        if(pconfig->strict_mode)
            enableStrictMode(curr_group);
        else
            disableStrictMode(curr_group);
    } else if (MATCH("security", "change_owner")) {
        if(openGroup(curr_group) < 0)
            return -1;        
        
        buffer = strdup(value);  
        sscanf(buffer, "%ud", &owner);
        free(buffer);

        changeOwner(curr_group, owner);
    } else if (MATCH("testing", "fork")) {
        int i;
        len = atoi(value);

        for(i=0; i<len; i++)
            fork();
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}



int loadConfig(char *config_path, thread_synch_t *main_synch){
    configuration config;

    main_syncher = main_synch;

    if (ini_parse(config_path, handler, &config) < 0) {
        printf("Can't load '%s'\n", config_path);
        return 1;
    }

    return 0;
}