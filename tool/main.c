#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> //For sleep()/nanosleep()/getppid()


#include "../lib/thread_synch.h"



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


extern int loadConfig(char *config_path, thread_synch_t *main_synch);

void pause_char(){
    //printf("\n\nWaiting...\n");
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


int closeLoadedGroups(){
    int i;
    for(i=0; i<MAX_GROUPS; i++){
        if(groups[i] == NULL)   
            continue;
        else
            close(groups[i]->file_descriptor);
    }
}



int groupSubMenu(thread_group_t group){
    int choice;
    int exit_flag = 0;
    int ret = 99;
    long delay;
    unsigned long param_value;
    long storage_size;
    uid_t new_owner;

    char *buffer;
    size_t buff_size;
    
    int ratio;

    if( (ret = openGroup(&group)) < 0){
        if(ret == PERMISSION_ERR){
            printf("\nudev daemon has not changed permission yet, retrying...\n");

            sleep(1);

            if( (ret = openGroup(&group))){
                printf("\nUnable to reload the module. Error code: %d\n", ret);
                return -1;
            }

        }else{
            printf("\nError while opening the group. Code: %d\n", ret);
            return -1;
        }
    }

    clear();
 
    do{
        printf(ANSI_COLOR_YELLOW "\n[Group %d Management]\n" ANSI_COLOR_RESET, group.group_id);

        printf("Select Options:\n\t1 - Read\n\t2 - Write\n\t3 - Set Delay\n\t"
            "4 - Revoke Delay\n\t5 - Flush\n\t6 - Sleep on Barrier\n\t7 - Awake barrier"
            "\n -Message Param\n\t80 - Display Parameters\n\t81 - Set max message size\n\t"
            "82 - Set max storage size\n\t83 - Set Garbage Collector ratio\n\t"
            "91 - (Dis)Enable strict mode\n\t92 - Change Owner\n\t"
            "99 - Exit\n:");

        scanf(" %d", &choice);

        clear();


            switch (choice){
            case 1:     //Read

                printf("\nSize to read: ");
                scanf(" %lu", &buff_size);

                buffer = (char*)malloc( sizeof(char)*buff_size);
                if(!buffer){
                    printf(ANSI_COLOR_RED "\n[X] Memory allocation problem" ANSI_COLOR_RESET);
                    break;
                }

                ret = readMessage(buffer, buff_size, &group);

                if(ret < 0){
                    printf(ANSI_COLOR_RED "\n[X] Error while reading the message: %d\n" ANSI_COLOR_RESET, ret);
                }else if(ret == 1){
                    printf(ANSI_COLOR_YELLOW "\nNo message present\n" ANSI_COLOR_RESET);
                }else{
                    printf(ANSI_COLOR_YELLOW "\nReaded message" ANSI_COLOR_RESET ": %s\n", buffer);
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
                
                //printf("\nUnimplemented");
                if(cancelDelay(&group) < 0)
                    printf(ANSI_COLOR_RED "\n[X] Error while flushing the messages" ANSI_COLOR_RESET);
                
                break;
            case 6: //Sleep on barrier
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
            case 80: //Display Parameters

                storage_size = getCurrentStorageSize(&group);

                //See 'getCurrentStorageSize' bug
                if(storage_size == 0L){
                    storage_size = getCurrentStorageSize(&group);
                }
                    

                printf("Current storage size: %lu\n", storage_size);
                printf("Max storage size: %lu\n", getMaxStorageSize(&group));
                printf("Max message size: %lu\n", getMaxMessageSize(&group));
                pause_char();
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
            case 83:    //Set Garbage collector ratio
                printf("\nGarbage collector ratio: ");
                scanf("%lu", &param_value);
                ret = setGarbageCollectorRatio(&group, param_value);
                break;  
            case 91:    //Enable/Disable strict mode

                printf("\n\t1 - Enable strict mode\n\t2 - Disable strict mode\n\t 99 - Back\n\n:");
                scanf(" %d", &choice);

                if(choice == 1)
                    ret = enableStrictMode(&group);
                else if (choice == 2)
                    ret = disableStrictMode(&group);
                else 
                    ret = 0;

                if(ret == UNAUTHORIZED)
                    printf(ANSI_COLOR_RED "\n[X] UNAUTHORIZED\n" ANSI_COLOR_RESET);

                break;

            case 92:    //Change Owner

                printf("\nInsert new owner UID: ");
                scanf(" %ud", (unsigned int*) &new_owner);

                ret = changeOwner(&group, new_owner);

                if(ret == UNAUTHORIZED)
                    printf(ANSI_COLOR_RED "\n[X] UNAUTHORIZED\n" ANSI_COLOR_RESET);
                else if(ret == 0)
                    printf("\nSuccessfully change owner of the group\n");

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

        
    }while(exit_flag == 0);



    closeLoadedGroups();


}


int main(int argc, char *argv[]){

    if(initThreadSyncher(&main_synch) < 0){
        printf("Unable to open main_syncher, exiting...\n");
        return -1;
    }

    if(argc > 1){
        loadConfig(argv[1], &main_synch);
    }else{
        interactiveSession();
    }

    return 0;
}
