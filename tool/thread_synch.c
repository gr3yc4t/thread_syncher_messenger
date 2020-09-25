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

//Parameter Read Functions

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


    ret = snprintf(param_path, BUFF_SIZE, param_default_path, group_id);

    if(ret < 0 || ret > BUFF_SIZE)
        return -1;

    printf("\nFirst string: %s", param_path);

    strncat(param_path, param_name, BUFF_SIZE);

    printf("\nSecond string: %s", param_path);


    fd = fopen(param_path, "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    printf("Reading param 'max_message_size'");
    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parameters: %d", ret);
        return -1;
    }

    printf("\n\nParam value %s", buffer);

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


    ret = snprintf(param_path, BUFF_SIZE, param_default_path, group_id);

    if(ret < 0 || ret > BUFF_SIZE)
        return -1;

    printf("\nFirst string: %s", param_path);

    strncat(param_path, param_name, BUFF_SIZE);

    printf("\nSecond string: %s", param_path);


    fd = fopen(param_path, "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    printf("Reading param 'max_message_size'");
    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parametersaaa: %d", ret);
        return -1;
    }

    *_val = strtoul(buffer, NULL, 10);

    fclose(fd);

    return 0;
}

static int _readCurrStorageSize(unsigned long *_val, const int group_id){
    FILE *fd;
    int ret;
    const char *param_name = "current_storage_size\0";
    char param_path[BUFF_SIZE];
    char buffer[BUFF_SIZE];

    char *final_string = NULL;

    printf("\nZero string: %s", param_default_path);

    ret = snprintf(param_path, BUFF_SIZE, "/sys/class/group_synch/group%d/message_param/", group_id);

    if(ret < 0 || ret > BUFF_SIZE)
        return -1;

    printf("\nFirst string: %s", param_path);

    if (asprintf(&final_string, "%s%s", param_path, "current_storage_size\0") == -1){
        printf("\nError while reading parameter");
        return -1;
    }




    //strcat(param_path, "current_storage_size\0");

    printf("\nSecond string: %s", final_string);


    fd = fopen(final_string, "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    printf("Reading param 'max_message_size'");
    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parametersaaa: %d", ret);
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


    ret = snprintf(param_path, BUFF_SIZE, param_default_path, group_id);

    if(ret < 0 || ret > BUFF_SIZE)
        return -1;

    printf("\nFirst string: %s", param_path);

    strncat(param_path, param_name, BUFF_SIZE);

    printf("\nSecond string: %s", param_path);


    fd = fopen(param_path, "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    printf("Reading param 'max_message_size'");
    ret = fread(buffer, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parametersaaa: %d", ret);
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
        printf("Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("Error while converting the paramtere value");
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
        printf("Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("Error while converting the paramtere value");
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
        printf("Error while opening the group file\n");
        return -1;
    }


    char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("Error while converting the paramtere value");
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

    printf("\nGroup id ioctl: %d", group_id);

    if(group_id < 0)
        return -1;      

    return group_id;    
}




int initThreadSycher(thread_synch_t *main_syncher){

    char *main_device_default_path = "/dev/main_thread_sync0";  //TODO: remove the zero
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

    if(group->file_descriptor < 0){
        printf("\nGroup path: %s\n", group->group_path);    
        fd = open(group->group_path, O_RDWR);

        if(fd < 0){
            group->file_descriptor = -1;

            switch (errno)
            {
            case EACCES:
                printf("\nPermission denied\n");
                
                break;
            
            default:
                break;
            }
            printf("\nUnable to open the group: %d", errno);
            return -1;
        }


        group->file_descriptor = fd;
    }

    return 0;
}

thread_group_t* installGroup(const group_t group_descriptor, thread_synch_t *main_synch){
    
    thread_group_t *new_group;
    group_t tmp_group;
    char *default_group_path = "/dev/group%d";
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

    max_size = strnlen(default_group_path, DEVICE_NAME_SIZE) + 4;   //Max group ID=255

    printf("\nMax size: %ud", max_size);    

    new_group->group_path = (char*)malloc(sizeof(char)*max_size);
    if(!new_group->group_path)
        goto cleanup1;

    printf("\nGenerating device path\n");

    ret = snprintf(new_group->group_path, DEVICE_NAME_SIZE, "/dev/group%d", group_id);

    if(ret < 0 || ret >= DEVICE_NAME_SIZE)
        goto cleanup2;

    printf("\nDevice path: %s", new_group->group_path);

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

int readMessage(void *buffer, size_t len, thread_group_t *group){

    int ret;

    if(group->file_descriptor == -1)
        return -1;


    ret = read(group->file_descriptor, buffer, sizeof(u_int8_t)*len);

    if(ret == 0)
        return 1;
    else if(ret < 0)
        return -1;

    return 0;
}

int writeMessage(const void *buffer, size_t len, thread_group_t *group){

    int ret;

    if(group->file_descriptor == -1){
        printf("Group not opened");
        return -1;
    }

    ret = write(group->file_descriptor, buffer, sizeof(u_int8_t)*len);
    printf("\nReturn: %d\n", ret);
    return ret;
}

int setDelay(const long _delay, thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return -1;
    
    ret = ioctl(group->file_descriptor, IOCTL_SET_SEND_DELAY, _delay);

    return ret;
}

int revokeDelay(thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return -1;
    
    ret = ioctl(group->file_descriptor, IOCTL_REVOKE_DELAYED_MESSAGES);

    return ret;  
}

int sleepOnBarrier(thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    ret = ioctl(group->file_descriptor, IOCTL_SLEEP_ON_BARRIER);

    return ret;
}

int awakeBarrier(thread_group_t *group){
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    ret = ioctl(group->file_descriptor, IOCTL_AWAKE_BARRIER);

    return ret;    
}

unsigned long getMaxMessageSize(thread_group_t *group){
    unsigned long val;
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    ret = _readMaxMsgSize(&val, group->group_id);

    if(ret < 0)
        val = 0L;

    return val;
}

unsigned long getMaxStorageSize(thread_group_t *group){
    unsigned long val;
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    ret = _readMaxStorageSize(&val, group->group_id);

    if(ret < 0)
        val = 0L;

    return val;
}

unsigned long getCurrentStorageSize(thread_group_t *group){
    unsigned long val;
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    ret = _readCurrStorageSize(&val, group->group_id);

    if(ret < 0)
        val = 0L;

    return val;
}

int readGroupInfo(thread_synch_t *main_syncher){
    int ret;
    char buffer[512];


    if(main_syncher->initialized != 1)
        return -1;


    lseek(main_syncher->main_file_descriptor, 1L, SEEK_CUR);
    
    ret = read(main_syncher->main_file_descriptor, buffer, 512);

    if(ret < 0)
        return -1;

    printf("\nGroup name: %s", buffer);

    return 0;
}


int setMaxMessageSize(thread_group_t *group, unsigned long val){
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    if(_setMaxMsgSize(group->group_id, val) < 0)
        return -1;
    return 0;
}

int setMaxStorageSize(thread_group_t *group, unsigned long val){
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    if(_setMaxStorageSize(group->group_id, val) < 0)
        return -1;
    return 0;
}

int setGarbageCollectorRatio(thread_group_t *group, unsigned long val){
    int ret;

    if(group->file_descriptor == -1)
        return -1;

    if(_setGarbCollRatio(group->group_id, val) < 0)
        return -1;
    return 0;
}


int _getDescriptorFromID(const int group_fd, group_t *descriptor){
    int ret;

    if(group_fd == -1)
        return -1;


    ret = ioctl(group_fd, IOCTL_GET_GROUP_DESC, descriptor);

    return ret;
}


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

    printf("\nFetched group ID: %d", group_id);

    if(group_id < 0)
        goto cleanup2;



    group->descriptor = tmp_group;
    group->group_id = group_id;
    

    ret = snprintf(group_path, DEVICE_NAME_SIZE, "/dev/group%d", group_id);
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






thread_group_t* loadGroupFromID(const int group_id){
    const char *default_path = "/dev/group%d";
    char group_path[BUFF_SIZE];
    int fd;
    group_t group_descriptor;
    thread_group_t *group;


    if(group_id < 0)
        return NULL;


    snprintf(group_path, BUFF_SIZE, default_path, group_id);


    fd = open(group_path, O_RDWR);

    if(fd < 0){
        printf("\nUnable to open the file: %d", errno);
        return NULL;
    }

    /*
    if(_getDescriptorFromID(fd, &group_descriptor) < 0){
        printf("\nIOCTL call error");
        return NULL;
    }*/



    group = (thread_group_t*)malloc(sizeof(thread_group_t));

    if(!group)
        return NULL;

    group->file_descriptor = fd;
    group->group_id = group_id;
    group->descriptor.group_name = NULL;
    group->descriptor.name_len = 0;
    group->group_path = group_path;
    group->path_len = strnlen(group_path, BUFF_SIZE);

    return group;
}