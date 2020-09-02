#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> //For sleep()/nanosleep()

#include <sys/types.h>
#include <sys/ioctl.h>

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)

#define THREAD_NUM 64

#define NO_MSG_PRESENT -10
#define MSG_INVALID_FORMAT -11
#define MSG_SIZE_ERROR  -12
#define MEMORY_ERROR -13


typedef struct t_message{
    pid_t author;   //Author thread

    void *buffer;

    ssize_t size;
    ssize_t count;
} msg_t;


typedef struct group_t {
	unsigned int group_id;		//Thread group ID
	char *group_name;
} group_t;



pthread_t tid[THREAD_NUM];


void t_nanosleep(long _nanoseconds){

    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = _nanoseconds
    };

    struct timespec interval2;

    nanosleep(&interval, &interval2);

}



void concurrentRead(void *args){

    pthread_t id = pthread_self();
    printf("\n[R] Thread N. %ld\n", id);

    FILE *fd;
    fd = fopen((char*)args, "r"); 

    if(fd == NULL){
        printf("[T-%ld] Error while opening the group file\n", id);
        return -1;
    }

    //Random Sleep 1
    long sleep_time1 = rand()%10000;
    usleep(sleep_time1);



    msg_t msg;
    int ret = fread(&msg, sizeof(msg_t), 1, fd);

    if(!ret){
        switch (ret)
        {
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

    fclose(fd);

}


void concurrentWrite(void *args){

    pthread_t id = pthread_self();
    printf("\n[w] Thread N. %ld\n", id);

    char buffer[256];
    FILE *fd;


    strcpy(buffer, "PIPPO\0");
    int len = strlen(buffer);

    printf("\nLen = %d\n", len);

    msg_t my_msg = {
        .author = getppid(),
        .size = sizeof(char),
        .count = len,
        .buffer = &buffer
    };


    fd = fopen((char*)args, "w"); 

    if(fd == NULL){
        printf("Error while opening the group file\n");
        return -1;
    }


    int ret = fwrite(&my_msg, sizeof(msg_t), 1, fd);

    printf("Totel element written: %ld", ret);
    printf("\nsizeof(msg_t) = %ld", sizeof(msg_t));


    //Random Sleep
    long sleep_time = rand()%10000;
    usleep(sleep_time);


    fclose(fd);

    return;

}

int installGroup(const char *main_device_path){

    FILE *fd;

    fd = fopen(main_device_path, "rw"); 

    if(fd == NULL){
        printf("Error while opening the main_device file\n");
        return -1;
    }


    int numeric_descriptor = fileno(fd);


    //IOCTL request

    group_t new_group;

    new_group.group_id = 99;
    new_group.group_name = "pippo";

    int ret = ioctl(numeric_descriptor, IOCTL_INSTALL_GROUP, &new_group);

    printf("\n\nReturn code : %d\n\n", ret);


    fclose(fd);
}



int readGroup(const char *group_path){

    FILE *fd;

    fd = fopen(group_path, "r"); 

    if(fd == NULL){
        printf("Error while opening the group file\n");
        return -1;
    }

    int numeric_descriptor = fileno(fd);
    msg_t msg;

    int ret = fread(&msg, sizeof(msg_t), 1, fd);

    if(ret <= 0){
        printf("[ ] No message is present\n");
    }

    printf("Buffer content: %s\n", msg.buffer);

    return 0;

}


int writeGroup(const char *group_path){

    char buffer[256];

    strcpy(buffer, "PIPPO\0");

    int len = strlen(buffer);

    printf("\nLen = %d\n", len);

    msg_t my_msg = {
        .author = getppid(),
        .size = sizeof(char),
        .count = len,
        .buffer = &buffer
    };



    FILE *fd;

    fd = fopen(group_path, "w"); 

    if(fd == NULL){
        printf("Error while opening the group file\n");
        return -1;
    }


    int ret = fwrite(&my_msg, sizeof(msg_t), 1, fd);


    printf("Totel element written: %ld", ret);
    printf("\nsizeof(msg_t) = %ld", sizeof(msg_t));

    fclose(fd);

    return;
}




int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s install main_device\n", argv[0]);
        printf("Usage: %s read group_device\n", argv[0]);
        return -1;
    }


    if(strcmp(argv[1], "install") == 0){
        installGroup(argv[2]);
    }else if(strcmp(argv[1], "read") == 0){
        readGroup(argv[2]);
    }else if(strcmp(argv[1], "write") == 0){
        writeGroup(argv[2]);
    }else if(strcmp(argv[1], "wr") == 0){
        writeGroup(argv[2]);
        printf("\nGroup written, waiting for a char to continue...\n");
        int c = getchar();
        readGroup(argv[2]);
    }else if(strcmp(argv[1], "cwr") == 0){
        int i, err;

        for(i=0; i<THREAD_NUM; i++){
            
            if(i%2 == 0)
                err = pthread_create(&(tid[i]), NULL, &concurrentRead, (void*)argv[2]);
            else
                err = pthread_create(&(tid[i]), NULL, &concurrentWrite, (void*)argv[2]);

            if(err < 0){
                printf("\n[%d] Errror while creating the thread", i);
            }
        }


        for(i=0; i<THREAD_NUM; i++){
            long t_sleep = rand()%10000;
            t_nanosleep(t_sleep);

            err = pthread_join(tid[i], NULL);
            if(err < 0){
                printf("\n[%d] Errror while joining the thread", i);
            }
        }
        
        printf("\n\nTerminated\n\n");

    }else{
        printf("Unimplemented\n\n");
    }


    return 0;
}
