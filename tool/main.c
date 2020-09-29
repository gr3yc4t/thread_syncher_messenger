#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> //For sleep()/nanosleep()/getppid()


#include "thread_synch.h"
#define IOCTL_CHANGE_OWNER _IOW('Q', 102, uid_t)
#define IOCTL_SET_STRICT_MODE _IOW('Q', 101, bool)


//Colors
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


//Configurations
#define THREAD_NUM 64
#define BUFF_SIZE 512
#define MAX_GROUPS 64

//Global var describing threads
pthread_t tid[THREAD_NUM];


thread_synch_t main_synch;
thread_group_t *groups[MAX_GROUPS];
int group_index = 0;




void pause_char(){
    printf("\n\nWaiting...\n");
    char temp = getchar();
}

void clear(){
    #if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        system("clear");
    #endif

    #if defined(_WIN32) || defined(_WIN64)
        system("cls");
    #endif
}

void t_nanosleep(long _nanoseconds){

    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = _nanoseconds
    };

    struct timespec interval2;

    nanosleep(&interval, &interval2);

}


int printGroupParams(){
    int ret;

    unsigned long max_message_size = 0;
    unsigned long max_storage_size = 0;
    unsigned long curr_storage_size = 0;

    /*
    if(_readMaxMsgSize(&max_message_size) < 0)
        return -1;
    
    if(_readMaxStorageSize(&max_storage_size) < 0)
        return -1;
    
    if(_readCurrStorageSize(&curr_storage_size) < 0)
        return -1;
    */
    printf("\nGroup 0 Data\n\n\tMax message size:  %ld\n\tMax Storage Size: %ld\n\tCurrent storage size: %ld\n\n", max_message_size, max_storage_size, curr_storage_size);

    return 0;
}

int setStrictMode(int fd, const int _value){
    int value = 0;
    if(_value > 1)
        value = 1;
    if(_value < 0)
        value = 0;

    return ioctl(fd, IOCTL_SET_STRICT_MODE, value);
}
int changeGroupOwner(int fd, uid_t new_owner){
    return ioctl(fd, IOCTL_CHANGE_OWNER, new_owner);
}
int flushMessages(const char *group_path){

    FILE *fd;

    fd = fopen(group_path, "rw"); 

    if(fd == NULL){
        printf("Error while opening the main_device file\n");
        return -1;
    }


    int ret = fflush(fd);

    printf("\n\nReturn code : %d\n\n", ret);


    fclose(fd);
}

void concurrentRead(void *args){
    const char *default_message = "TH-NUM-%lu\0";   
    char buffer[BUFF_SIZE];   
    size_t len;
    pthread_t id = pthread_self();

    printf("\n[R] Thread N. %ld\n", id);

    int *fd;
    fd = open((char*)args, O_RDONLY); 

    if(fd < 0){
        printf("[T-%ld] Error while opening the group file\n", id);
        return -1;
    }


    //Random Sleep 1
    long sleep_time1 = rand()%10000;
    usleep(sleep_time1);


    int ret = read(fd, &buffer, sizeof(char)*len);

    if(!ret){
        switch (ret){
        case NO_MSG_PRESENT:
            printf("[T-%ld/R] No msg. present\n", id);
            break;
        case MSG_SIZE_ERROR:
            printf("[T-%ld/R] Message too large for current limits\n", id);
            break;        
        default:
            printf("[T-%ld/R] Unknow error\n", id);
            break;
        }
    }

    //Random Sleep 2
    long sleep_time2 = rand()%10000;
    usleep(sleep_time2);

    close(fd);

}

void concurrentWrite(void *args){

    const char *default_message = "TH-NUM-%lu\0";
    pthread_t id = pthread_self();
    char buffer[BUFF_SIZE];
    size_t len;

    printf("\n[w] Thread N. %ld\n", id);

    snprintf(buffer, BUFF_SIZE, default_message, id);
    len = strnlen(buffer, BUFF_SIZE);


    int *fd;
    fd = open((char*)args, O_WRONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    printf("\nLen = %d\n", len);

    int ret = write(fd, &buffer, sizeof(char)*len);

    printf("Totel element written: %ld", ret);


    //Random Sleep
    long sleep_time = rand()%10000;
    usleep(sleep_time);


    close(fd);

    return;

}



int showLoadedGroups(){
    int i;

    for(i=0; i<MAX_GROUPS; i++){

        if(groups[i] == NULL){
            continue;
        }else{

            if(groups[i]->descriptor.group_name == NULL)
                printf("\n[" ANSI_COLOR_YELLOW "%d" ANSI_COLOR_RESET "]\tGroup ID: %d\n", i, groups[i]->group_id);
            else
                printf("\n[" ANSI_COLOR_YELLOW "%d" ANSI_COLOR_RESET "]\tGroup name: %s\n", i, groups[i]->descriptor.group_name);
        }

    }

}



int groupSubMenu(thread_group_t group){
    int choice;
    int exit_flag = 0;
    int ret = 99;
    long delay;
    unsigned long param_value;
    long storage_size;

    char *buffer;
    size_t buff_size;

    
    int ratio;

    if(openGroup(&group) < 0){
        printf("\nError while opening the group\n");
        return -1;
    }

    clear();
 
    do{
        printf("\n[Group %d Management]\n", group.group_id);

        printf("Current storage size: %lu\n", getCurrentStorageSize(&group));
        printf("Max storage size: %lu\n", getMaxStorageSize(&group));
        printf("Max message size: %lu\n", getMaxMessageSize(&group));

        printf("Select Options:\n\t1 - Read\n\t2 - Write\n\t3 - Set Delay\n\t"
            "4 - Revoke Delay\n\t5 - Flush\n\t6 - Sleep on Barrier\n\t7 - Awake barrier"
            "\n -Message Param -\n\t 80 - Show Message Param \n\t81 - Set max message size\n\t"
            "82 - Set max storage size\n\t83 - Set Garbage Collector ratio\n\t"
            "99 - Exit\n:");

        scanf(" %d", &choice);

        clear();


            switch (choice){
            case 1:     //Read

                printf("\nSize to read: ");
                scanf("%ud", &buff_size);

                buffer = (char*)malloc(sizeof(char)*buff_size);

                ret = readMessage(buffer, buff_size, &group);

                if(ret < 0){
                    printf(ANSI_COLOR_RED "\n[X] Error while reading the message\n" ANSI_COLOR_RESET);
                }else if(ret == 1){
                    printf(ANSI_COLOR_YELLOW "\nNo message present\n" ANSI_COLOR_RESET);
                }else{
                    printf(ANSI_COLOR_YELLOW "\nReaded message\n" ANSI_COLOR_RESET ": %s", buffer);
                }

                free(buffer);

                break;
            case 2:     //Write

                buffer = (char*)malloc(sizeof(BUFF_SIZE));
                if(!buffer)
                    break;

                printf("\nContent: ");
                if(scanf(" %s", buffer) > BUFF_SIZE){
                    printf(ANSI_COLOR_RED "\n[X] Size too large" ANSI_COLOR_RESET);
                    free(buffer);
                    break;
                }
                
                buff_size = strlen(buffer);
                
                if(writeMessage(buffer, buff_size, &group) < 0)
                    printf(ANSI_COLOR_RED "\n[X] Error while writing the message" ANSI_COLOR_RESET);

                free(buffer);
                break;
            case 3: //Set Delay

                buffer = (char*)malloc(sizeof(BUFF_SIZE));
                if(!buffer)
                    break;            

                printf("\nDelay Value: ");
                if(scanf(" %s", buffer) > BUFF_SIZE){
                    printf(ANSI_COLOR_RED "\n[X] Size too large" ANSI_COLOR_RESET);
                    free(buffer);
                    break;
                }

                delay = strtol(buffer, NULL, 10);

                if(setDelay(delay, &group) < 0)
                    printf(ANSI_COLOR_RED "\n[X] Error while setting the delay" ANSI_COLOR_RESET);

                free(buffer);

                break;
            case 4: //Revoke Delay

                if(revokeDelay(&group) < 0){
                    printf(ANSI_COLOR_RED "\n[X] Error while revoking the delay" ANSI_COLOR_RESET);
                }else{
                    printf("\nDelayed message revoked");
                }
                break;
            case 5: //Flush
                printf("\nUnimplemented");
                break;
            case 6: //Sleep on barrier
                printf("\nThread is going to sleep...");
                if(sleepOnBarrier(&group) < 0){
                    printf(ANSI_COLOR_RED "\n[X] Error while sleeping" ANSI_COLOR_RESET);
                    break;
                }
                printf("\nThread awaken!!!");
                break;
            case 7: //Awake barrier
                if(awakeBarrier(&group) < 0){
                    printf(ANSI_COLOR_RED "\n[X] Error while awaken threads" ANSI_COLOR_RESET);
                    break;
                }
                printf("\nBarrier Awaked!");
                break;
            case 80:
                if(printGroupParams() < 0)
                    printf(ANSI_COLOR_RED "[X] Error while reading parameters\n" ANSI_COLOR_RESET);
                break;
            case 81:
                printf("\nMax size value: ");
                scanf("%lu", &param_value);
                ret = setMaxMessageSize(&group, param_value);
                break;
            case 82:
                printf("\nMax storage value: ");
                scanf("%lu", &param_value);
                ret = setMaxStorageSize(&group, param_value);   
                break;
            case 83:
                printf("\nGarbage collector ratio: ");
                scanf("%lu", &param_value);
                ret = setGarbageCollectorRatio(&group, param_value);

                break;        
            case 99:
                printf("\n\nExiting...\n");
                exit_flag = 1;
                break;
            default:
                printf(ANSI_COLOR_RED "\n\nInvalid command\n\n" ANSI_COLOR_RESET);
                ret = -1;
                break;
            }

            printf("Returned Value: %d\n", ret);

    }while(exit_flag == 0);
}



int interactiveSession(){

    int exit_flag = 0;
    int choice;
    char group_name[256];
    size_t len;
    group_t descriptor;
    thread_group_t *tmp_group;
    int i;


    if(initThreadSycher(&main_synch) < 0){
        printf("Unable to open main_syncher, exiting...\n");
        return -1;
    }


    for(i=0; i<MAX_GROUPS; i++)
        groups[i] = NULL;


    do{
        printf("\n\n1- Install group\n2- Load Group from ID\n"
        "3- Load group from name\n4- Load group from memory\n99- Exit\n");
        
        showLoadedGroups();
        
        printf("\n:");
        scanf(" %d", &choice);

        switch (choice){
        case 1: //Install Group
            printf("\nInsert group name: ");
            scanf("%s", group_name);

            descriptor.group_name = (char*)malloc(sizeof(char)*strlen(group_name)+1);

            if(!descriptor.group_name){
                printf("\nAllocation error");
                break;
            }

            descriptor.name_len = strlen(group_name);
            strncpy(descriptor.group_name, group_name, descriptor.name_len);
            
            printf("\nInstalling group [%s]...", descriptor.group_name);

            groups[group_index] = installGroup(descriptor, &main_synch);

            printf("\nGroup installed");

            if(!groups[group_index]){
                printf("\nError while installing the group");
            }else{
                printf("\nGroup installed!!\n");
                
                groupSubMenu(*groups[group_index]);
                group_index++;

            }
            break;
        
        case 2: //Load group from ID
            printf("\nInsert group ID: ");
            scanf(" %d", &choice);

            groups[group_index] = loadGroupFromID(choice);
            if(!groups[group_index]){
                printf("\nUnable to load group");
                break;
            }

            printf("\nSuccessfully loaded group");
            group_index++;

            break;
        case 3: //Load group from name
            printf("\nInsert group name: ");
            scanf("%s", group_name);

            len = strlen(group_name);

            descriptor.group_name = (char*)malloc(sizeof(char)*len);

            strncpy(descriptor.group_name, group_name, len);
            descriptor.name_len = len;

            groups[group_index] = loadGroupFromDescriptor(&descriptor, &main_synch);

            if(groups[group_index] == NULL)
                printf("Error while creating the group");
            else{
                printf("\nGroup created correctly");
                group_index++;
            }

            break;
        case 4: //Load group from memory
            printf("\nInsert group index: ");
            scanf("%d", &choice);

            if(choice > group_index)
                choice = group_index-1;

            if(groups[choice] == NULL)
                break;

            groupSubMenu(*groups[choice]);

            break;
        case 99:
            exit_flag = 1;
        default:
            printf("\nInvalid command\n");
            break;
        }

        
        //readGroupInfo(&main_synch);

    }while(exit_flag == 0);


}


int main(int argc, char *argv[]){

    interactiveSession();

    return 0;
}
