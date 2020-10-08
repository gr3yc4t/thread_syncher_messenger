#include "thread_synch.h"
#include <errno.h>

static int _installGroup(group_t *new_group, thread_synch_t *main_synch){

    int group_id;

    if(!main_synch->initialized)
        return -1;

    if(new_group->group_name == NULL || new_group->name_len == 0)
        return -1;

    group_id = ioctl(main_synch->main_file_descriptor, IOCTL_INSTALL_GROUP, new_group);

    if(group_id < 0)
        return -1;

    return group_id;
}

static int _getParamPath(const int group_id, const char *param_name, char *dest_buffer, size_t dest_size){
    char param_path[BUFF_SIZE];
    int ret;
    size_t len;
    
    ret = snprintf(param_path, BUFF_SIZE, param_default_path, group_id);

    strncat(param_path, param_name, BUFF_SIZE);

    len = strnlen(param_path, BUFF_SIZE);

    if(dest_size < len)
        return -1;

    strncpy(dest_buffer, param_path, dest_size);

    return 0;
}

static int _readMaxMsgSize(unsigned long *_val, const int group_id){
    FILE *fd;
    int ret;
    const char *param_name = "max_message_size";
    char param_path[BUFF_SIZE];
    char buffer[BUFF_SIZE];


    if(_getParamPath(group_id, param_name, param_path, BUFF_SIZE) < 0)
        return -1;


    fd = fopen(param_path, "rt"); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("[X] Errror while reading parameters: %d\n", ret);
        return -1;
    }

    *_val = strtoul(buffer, NULL, 10);

    fclose(fd);

    return 0;
}

static int _readMaxStorageSize(unsigned long *_val, const int group_id){
    FILE *fd;
    int ret;
    const char *param_name = "max_storage_size";
    char param_path[BUFF_SIZE];
    char buffer[BUFF_SIZE];


    if(_getParamPath(group_id, param_name, param_path, BUFF_SIZE) < 0)
        return -1;


    fd = fopen(param_path, "rt"); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("[X] Errror while reading parameters: %d", ret);
        return -1;
    }

    *_val = strtoul(buffer, NULL, 10);

    fclose(fd);

    return 0;
}

static int _readCurrStorageSize(unsigned long *_val, const int group_id){
    FILE *fd;
    int ret;
    const char *param_name = "current_message_size";
    char param_path[BUFF_SIZE];
    char buffer[BUFF_SIZE];


    if(_getParamPath(group_id, param_name, param_path, BUFF_SIZE) < 0)
        return -1;



    if(ret < 0 || ret > BUFF_SIZE)
        return -1;


    fd = fopen(param_path, "rt"); 

    if(!fd){
        printf("[X] Error while opening the group file: %d\n", errno);
        return -1;
    }


    ret = fread(buffer, sizeof(char), 64, fd);
    if(ferror(fd) != 0){
        printf("[X] Errror while reading parameters: %d", ret);
        return -1;
    }

    *_val = strtoul(buffer, NULL, 10);

    fclose(fd);

    return 0;
}

static int _readGroupParameter(unsigned long *_val, const int group_id, const char *param_name){
    FILE *fd;
    int ret;
    char param_path[BUFF_SIZE];
    char buffer[BUFF_SIZE];


    if(_getParamPath(group_id, param_name, param_path, BUFF_SIZE) < 0)
        return -1;


    fd = fopen(param_path, "rt"); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("[X] Errror while reading parameters: %d\n", ret);
        return -1;
    }

    *_val = strtoul(buffer, NULL, 10);

    fclose(fd);

    return 0;

}

static int _setMaxMsgSize(const int group_id, const unsigned long _val){
    int fd;
    int ret;
    char param_path[BUFF_SIZE];

    if(_getParamPath(group_id, "max_message_size", param_path, BUFF_SIZE) < 0)
        return -1;

    fd = open(param_path, O_WRONLY); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("[X] Error while converting the paramtere value");
        return -1;
    }

    ret = write(fd, buff, sizeof(char)*strnlen(buff, BUFF_SIZE));

    return ret;
}

static int _setMaxStorageSize(const int group_id, const unsigned long _val){
    int fd;
    int ret;
    char param_path[BUFF_SIZE];

    if(_getParamPath(group_id, "max_storage_size", param_path, BUFF_SIZE) < 0)
        return -1;

    fd = open(param_path, O_WRONLY); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("[X] Error while converting the paramtere value");
        return -1;
    }

    ret = write(fd, buff, sizeof(char)*strnlen(buff, BUFF_SIZE));

    return ret;
}

static int _setGarbCollRatio(const int group_id, const unsigned long _val){
    int fd;
    int ret;
    char param_path[BUFF_SIZE];

    if(_getParamPath(group_id, "garbage_collector_ratio", param_path, BUFF_SIZE) < 0)
        return -1;

    fd = open(param_path, O_WRONLY); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%lu", _val) < 0){
        printf("[X] Error while converting the paramtere value");
        return -1;
    }
    ret = write(fd, buff, sizeof(char)*strnlen(buff, BUFF_SIZE));

    return ret;
}

static int _setStructSizeFlag(const int group_id, const bool _val){
    int fd;
    int ret;
    char param_path[BUFF_SIZE];

    if(_getParamPath(group_id, "include_struct_size", param_path, BUFF_SIZE) < 0)
        return -1;

    fd = open(param_path, O_WRONLY); 

    if(fd < 0){
        printf("[X] Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%d", (int)_val) < 0){
        printf("[X] Error while converting the paramtere value");
        return -1;
    }
    ret = write(fd, buff, sizeof(char)*strnlen(buff, BUFF_SIZE));

    return ret;    
}

static int _getGroupID(group_t *descriptor, thread_synch_t *main_synch){
    int group_id;

    if(!main_synch->initialized)
        return -1;

    if(descriptor->group_name == NULL || descriptor->name_len == 0)
        return -1;


    group_id = ioctl(main_synch->main_file_descriptor, IOCTL_GET_GROUP_ID, descriptor);

    if(group_id < 0)
        return -1;      

    return group_id;    
}

static int _getDescriptorFromID(const int group_fd, group_t *descriptor){
    int ret;

    if(group_fd == -1)
        return -1;


    ret = ioctl(group_fd, IOCTL_GET_GROUP_DESC, descriptor);

    return ret;
}

static int _setStrictMode(int fd, const int _value){
    int value = 0;
    if(_value >= 1)
        value = 1;
    if(_value < 0)
        value = 0;

    return ioctl(fd, IOCTL_SET_STRICT_MODE, value);
}

static int _changeGroupOwner(int fd, const uid_t new_owner){
    return ioctl(fd, IOCTL_CHANGE_OWNER, new_owner);
}


/**
 *  @brief Initialize a 'thread_synch_t' structure 
 *  
 *  @param[out] main_syncher A pointer to the structure to initialize
 *  
 *  @retval TS_NOT_FOUND if the main_syncher default file cannot be found
 *  @retval TS_OPEN_ERR if there was an error while opening the main_syncher file
 *  @retval ALLOC_ERR if there was an error during memory allocation
 *  @retval 0 on success
 *  
 * 
 */
int initThreadSyncher(thread_synch_t *main_syncher){

    char *main_device_default_path = "/dev/main_thread_synch";
    int path_len;
    char *path;
    int fd;

    if( access( main_device_default_path, W_OK ) == -1){
        return TS_NOT_FOUND;
    }

    fd = open(main_device_default_path, O_WRONLY);

    if(fd < 0){
        return TS_OPEN_ERR;
    }

    main_syncher->main_file_descriptor = fd;

    path_len = strlen(main_device_default_path);

    main_syncher->main_device_path = (char*)malloc(sizeof(char)*path_len+1);

    if(!main_syncher->main_device_path){
        return ALLOC_ERR;
    }


    strncpy(main_syncher->main_device_path, main_device_default_path, path_len);

    main_syncher->path_len = path_len;

    lseek(main_syncher->main_file_descriptor, 0L, SEEK_SET);

    main_syncher->initialized = 1;

    return 0;
}


/**
 * @brief Open a group, the thread PID is added to group's active members
 * 
 * @retval 0 on success
 * @retval PERMISSION_ERR on permission Error
 * @retval negative number on error
 * 
 */
int openGroup(thread_group_t* group){
    int fd;

    if(group == NULL)
        return -1;

    if(group->file_descriptor < 0){
        fd = open(group->group_path, O_RDWR);

        if(fd < 0){
            group->file_descriptor = -1;

            switch (errno){
            case EACCES:
                return PERMISSION_ERR;
            default:
                break;
            }
            return -errno;
        }

        group->file_descriptor = fd;
    }

    return 0;
}


/**
 * @brief Install a group in the system given a group descriptor
 * 
 * @param[in] group_descriptor The new group's descriptor
 * @param[in] main_synch A pointer to an initialized main_sych structure
 * 
 * @retval A pointer to a group structure
 * @retval NULL on error
 */

thread_group_t* installGroup(const group_t group_descriptor, thread_synch_t *main_synch){
    
    thread_group_t *new_group;
    group_t tmp_group;
    char buffer[DEVICE_NAME_SIZE];
    char *group_name;
    int group_id;
    unsigned int max_size;
    int ret;

    if(!main_synch)
        return NULL;

    tmp_group = group_descriptor;

    group_id = _installGroup(&tmp_group, main_synch);

    if(group_id < 0)
        return NULL;

    new_group = (thread_group_t*)malloc(sizeof(thread_group_t));

    if(!new_group)
        return NULL;

    new_group->group_id = group_id;

    max_size = strnlen(group_default_path, DEVICE_NAME_SIZE) + 4;   //Max group ID=255

    new_group->group_path = (char*)malloc(sizeof(char)*max_size);
    if(!new_group->group_path)
        goto cleanup1;

    ret = snprintf(new_group->group_path, DEVICE_NAME_SIZE, group_default_path, group_id);

    if(ret < 0 || ret >= DEVICE_NAME_SIZE)
        goto cleanup2;

    new_group->path_len = strnlen(new_group->group_path, DEVICE_NAME_SIZE);


    //Copy the group name locally to avoid that user free the buffer inside 'group_t'
    group_name = (char*)malloc(sizeof(char)*group_descriptor.name_len);

    if(!group_name)
        goto cleanup2;

    strncpy(group_name, group_descriptor.group_name, group_descriptor.name_len);

    
    new_group->descriptor.group_name = group_name;
    new_group->descriptor.name_len = group_descriptor.name_len;


    new_group->file_descriptor = -1;    //The user should open the file

    return new_group;

    cleanup2:
        free(new_group->group_path);
    cleanup1:
        free(new_group);
        return NULL;
    
}


/**
 * @brief Read a message from a given group
 * 
 * @param[out] buffer The buffer where the readed message is stored
 * @param[in]  len  The number of bytes to read
 * @param[in]  group A pointer to the group's structure where the msg is readed
 * 
 * @retval NO_MSG_PRESENT if there is no message available
 * @retval GROUP_CLOSED if the provided group is closed
 * @retval negative number on error
 * @retval 0 on success
 * 
 * @note Even if the parameter 'len' is less the the size of the actual message, the message 
 *          is considered delivered and thus removed from the queue
 */
int readMessage(void *buffer, size_t len, thread_group_t *group){

    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    if(!buffer)
        return -1;


    ret = read(group->file_descriptor, buffer, sizeof(u_int8_t)*len);

    if(ret == 0)
        return NO_MSG_PRESENT;
    else if(ret < 0)
        return -1;

    return 0;
}

/**
 * @brief Write a message in a given group
 * 
 * @param[in] buffer The buffer that contains the message to write
 * @param[in]  len  The number of bytes to write
 * @param[in]  group A pointer to the group's structure where the msg is written
 * 
 * @retval negative number on error
 * @retval The number of written bytes on successs 
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 */
int writeMessage(const void *buffer, size_t len, thread_group_t *group){

    int ret;
    
    if(group == NULL || group->file_descriptor == -1){
        return GROUP_CLOSED;
    }

    ret = write(group->file_descriptor, buffer, sizeof(u_int8_t)*len);
    return ret;
}

/**
 * @brief Set the delay value for a given group
 * 
 * @param[in] _delay The new delay value
 * @param[in] *group A pointer to an initialized group structure
 * 
 * @retval -1 on error
 * @retval 0 on success
 * @retval GROUP_CLOSED if the provided group is closed
 */
int setDelay(const long _delay, thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;
    
    ret = ioctl(group->file_descriptor, IOCTL_SET_SEND_DELAY, _delay);

    return ret;
}

/**
 * @brief Revoke all the delayed messaged on a given group and deliver it
 * 
 * @param[in] *group A pointer to an initialized group structure
 * 
 * @retval Negative number on error
 * @retval The number of revoked messages on success (>= 0)
 * @retval GROUP_CLOSED if the provided group is closed
 */
int revokeDelay(thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;
    
    ret = ioctl(group->file_descriptor, IOCTL_REVOKE_DELAYED_MESSAGES);

    return ret;  
}

/**
 * @brief Put the process which call this function on sleep
 * 
 * @param[in] *group T A pointer to an initialized group structure
 * 
 * @retval Negative number on error
 * @retval 0 on success
 * @retval GROUP_CLOSED if the provided group is closed
 */
int sleepOnBarrier(thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    ret = ioctl(group->file_descriptor, IOCTL_SLEEP_ON_BARRIER);

    return ret;
}

/**
 * @brief Awake all the process present in a wait queue for a given group
 * 
 * @param[in] *group T A pointer to an initialized group structure
 * 
 * @retval -1 on error
 * @retval 0 on success
 * @retval GROUP_CLOSED if the provided group is closed
 */
int awakeBarrier(thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    ret = ioctl(group->file_descriptor, IOCTL_AWAKE_BARRIER);

    return ret;    
}

/**
 * @brief Get the maximum message size value for a given group
 * 
 * @param[in] *group T A pointer to an initialized group structure
 * 
 * @retval The value of the parameter
 * @retval 0 on error
 * 
 */
unsigned long getMaxMessageSize(thread_group_t *group){
    unsigned long val;
    int ret;

    if(group->file_descriptor == -1)
        return 0L;

    ret = _readMaxMsgSize(&val, group->group_id);

    if(ret < 0)
        val = 0L;

    return val;
}

/**
 * @brief Get the maximum storage size value for a given group
 * 
 * @param[in] *group T A pointer to an initialized group structure
 * 
 * @retval The value of the parameter
 * @retval 0 on error
 * 
 */
unsigned long getMaxStorageSize(thread_group_t *group){
    unsigned long val;
    int ret;

    if(group->file_descriptor == -1)
        return 0L;

    ret = _readMaxStorageSize(&val, group->group_id);

    if(ret < 0)
        val = 0L;

    return val;
}

/**
 * @brief Get the current storage size value for a given group
 * 
 * @param[in] *group T A pointer to an initialized group structure
 * 
 * @bug Sometimes this function will return the incorrect value "0", when this
 *          happens a subsequent call of the function will return the correct value
 * 
 * @retval The value of the parameter
 * @retval 0 on error
 * 
 */
unsigned long getCurrentStorageSize(thread_group_t *group){
    unsigned long val;
    int ret;

    if(group->file_descriptor == -1)
        return 0L;

    ret = _readCurrStorageSize(&val, group->group_id);

    if(ret < 0)
        val = 0L;

    return val;
}




/**
 * @brief Set the maximum message size value for a given group
 * 
 * @param[in] *group A pointer to an initialized group structure
 * @param[in] val The new value of the parameter
 * 
 * @retval -1 on error
 * @retval 0 on success
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 */
int setMaxMessageSize(thread_group_t *group, const unsigned long val){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    if(_setMaxMsgSize(group->group_id, val) < 0)
        return -1;
    return 0;
}

/**
 * @brief Set the maximum storage size value for a given group
 * 
 * @param[in] *group A pointer to an initialized group structure
 * @param[in] val The new value of the parameter
 * 
 * @retval -1 on error
 * @retval 0 on success
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 */
int setMaxStorageSize(thread_group_t *group, const unsigned long val){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    if(_setMaxStorageSize(group->group_id, val) < 0)
        return -1;
    return 0;
}

/**
 * @brief Set the garbage collector ratio value for a given group
 * 
 * @param[in] *group A pointer to an initialized group structure
 * @param[in] val The new value of the parameter
 * 
 * @retval -1 on error
 * @retval GROUP_CLOSED if the provided group is closed
 * @retval 0 on success
 * 
 */
int setGarbageCollectorRatio(thread_group_t *group, const unsigned long val){
    int ret;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    if(_setGarbCollRatio(group->group_id, val) < 0)
        return -1;
    return 0;
}




/**
 * @brief Enable strict mode on a group
 * @param[in] *group A pointer to an initialized group structure
 * 
 * @retval 0 on success
 * @retval UNAUTHORIZED if the current user is not authorized to change the param
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 * @note If strict mode is disabled, any threads can enable it through this function. 
 *       If strict mode is enabled, only the owner can disable it
 */
int enableStrictMode(thread_group_t *group){
    int ret = 0;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    
    ret = _setStrictMode(group->file_descriptor, 1);

    if(ret < 0)
        return UNAUTHORIZED;
    
    return 0;
}

/**
 * @brief Disable strict mode on a group
 * @param[in] *group A pointer to an initialized group structure
 * 
 * @retval 0 on success
 * @retval UNAUTHORIZED if the current user is not authorized to change the param
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 * @note If strict mode is enabled, only the owner can disable it
 */
int disableStrictMode(thread_group_t *group){
    int ret = 0;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    
    ret = _setStrictMode(group->file_descriptor, 0);

    if(ret < 0)
        return UNAUTHORIZED;
    
    return 0;
}


/**
 * @brief Set a flag to count also supporting structure size in storage calculation
 * @param[in] *group A pointer to an initialized group structure
 * @param[in] value The value the flag will be set to
 * 
 * @retval 0 on success
 * @retval UNAUTHORIZED if the current user is not authorized to change the param
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 * @note If strict mode is enabled, only the owner can disable it
 */
int includeStructureSize(thread_group_t *group, const bool value){
    int ret = 0;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    ret = _setStructSizeFlag(group->file_descriptor, value);

    if(ret < 0)
        return UNAUTHORIZED;
    
    return 0;    
}



/**
 * @brief Change the owner of a group
 * @param[in] *group A pointer to an initialized group structure
 * 
 * @retval 0 on success
 * @retval UNAUTHORIZED if the current user is not authorized to change the param
 * @retval GROUP_CLOSED if the provided group is closed
 * 
 * @note If strict mode is enabled, only the current owner can set a new owner
 * @note If strict mode is enabled, any thread could change the group's owner
 */
int changeOwner(thread_group_t *group, const uid_t new_owner){
    int ret = 0;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    ret = _changeGroupOwner(group->file_descriptor, new_owner);

    if(ret < 0)
        return UNAUTHORIZED;
    
    return 0;
}

/**
 * @brief Change the owner of a group to the calling thread's UID
 * @param[in] *group A pointer to an initialized group structure
 * 
 * @retval 0 on success
 * @retval UNAUTHORIZED if the current user is not authorized to change the param
 * @retval Negative number on error
 * 
 * @note If strict mode is enabled, only the current owner can set a new owner
 * @note If strict mode is enabled, any thread could change the group's owner
 */
int becomeOwner(thread_group_t *group){
    return changeOwner(group, getuid());
}

/**
 * @brief Load a group's structure given a group descriptor
 * 
 * @param[in] *descriptor The descritor of the group to load
 * @param[in] *main_syncher Pointer to a thread_synch_t main structure
 * 
 * @retval A pointer to the group structure of the specified group
 * @retval NULL on error
 *  
 * @note The provided group structure is not opened by default
 */
thread_group_t *loadGroupFromDescriptor(const group_t *descriptor, thread_synch_t *main_syncher){
    char group_path[BUFF_SIZE];
    group_t tmp_group;
    thread_group_t *group;
    int group_id;
    size_t path_len;
    int ret;

    if(main_syncher->initialized == 0)
        return NULL;

    tmp_group.name_len = descriptor->name_len;
    tmp_group.group_name = (char*)malloc(sizeof(char)*tmp_group.name_len+1);

    if(!tmp_group.group_name)
        return NULL;

    strncpy(tmp_group.group_name, descriptor->group_name, tmp_group.name_len);


    group = (thread_group_t*)malloc(sizeof(thread_group_t));
    if(!group)
        goto cleanup1;


    group_id = _getGroupID(&tmp_group, main_syncher);

    if(group_id < 0)
        goto cleanup2;

    group->descriptor = tmp_group;
    group->group_id = group_id;
    

    ret = snprintf(group_path, DEVICE_NAME_SIZE, group_default_path, group_id);
    if(ret < 0 || ret >= DEVICE_NAME_SIZE)
        goto cleanup2;

    path_len = strnlen(group_path, DEVICE_NAME_SIZE);

    group->group_path = (char*)malloc(sizeof(char)*path_len);
    if(!group->group_path)
        goto cleanup1;


    strncpy(group->group_path, group_path, path_len);
    group->path_len = path_len;
    group->file_descriptor = -1;

    return group;

    cleanup2:
        free(group);
    cleanup1:
        free(tmp_group.group_name);
        return NULL;
}





/**
 * @brief Load a group structure given a group ID
 * 
 * @param group_id The group ID
 * @retval A pointer to the group structure of the specified group
 * @retval NULL on error
 * 
 * @note On success, the group is not opened 
 * @bug The group's descriptor is not loaded, thus the diplayed name will appear as "(null)"
 * 
 */
thread_group_t* loadGroupFromID(const int group_id){
    char group_path[BUFF_SIZE];
    int fd;
    group_t group_descriptor;
    thread_group_t *group;


    if(group_id < 0)
        return NULL;


    snprintf(group_path, BUFF_SIZE, group_default_path, group_id);


    fd = open(group_path, O_RDWR);

    if(fd < 0){
        printf("[X] Unable to open the file: %d\n", errno);
        return NULL;
    }

    group = (thread_group_t*)malloc(sizeof(thread_group_t));

    if(!group)
        return NULL;

    group->file_descriptor = fd;
    group->group_id = group_id;
    group->descriptor.group_name = NULL;
    group->descriptor.name_len = 0;
    group->path_len = strnlen(group_path, BUFF_SIZE);

    group->group_path = (char*)malloc(sizeof(char)*group->path_len);
    strncpy(group->group_path, group_path, group->path_len);

    return group;
}



/**
 * @brief Deliver immediately all delayed messages
 * 
 * When a group's device file is flushed (closed and opened) the driver
 * will cancel the delay on all messages present on the delay queue
 * 
 * @param[in] *group A pointer to a group's structure
 * 
 * @retval 0 on success
 * @retval GROUP_CLOSED if the provided group structure is closed
 * @retval negative number on error (the number represent the 'errno' value)
 */
int flushDelayedMsg(thread_group_t *group){
    int ret;
    int fd;

    if(group->file_descriptor == -1)
        return GROUP_CLOSED;

    close(group->file_descriptor);
    group->file_descriptor = -1;
    
    printf("\nPath: %s\n", group->group_path);

    fd = open(group->group_path, O_RDWR);

    if(fd < 0){
        group->file_descriptor = -1;
        printf("\nError number: %d", errno);
        return -errno;
    }
    
    group->file_descriptor = fd;

    return 0;
}