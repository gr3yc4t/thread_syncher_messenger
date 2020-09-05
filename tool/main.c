#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> //For sleep()/nanosleep()
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)
#define IOCTL_SET_SEND_DELAY _IOW('Y', 0, long)
#define IOCTL_REVOKE_DELAYED_MESSAGES _IO('Y', 1)


#define THREAD_NUM 64

#define NO_MSG_PRESENT -10
#define MSG_INVALID_FORMAT -11
#define MSG_SIZE_ERROR  -12
#define MEMORY_ERROR -13


typedef struct group_t {
	unsigned int group_id;		//Thread group ID
	char *group_name;
} group_t;



pthread_t tid[THREAD_NUM];


void pause_char(){
    printf("\n\nWaiting...\n");
    char temp = getchar();
}



void t_nanosleep(long _nanoseconds){

    struct timespec interval = {
        .tv_sec = 0,
        .tv_nsec = _nanoseconds
    };

    struct timespec interval2;

    nanosleep(&interval, &interval2);

}


int setDelay(const char *group_path, const long delay){

    int *fd;
    fd = open(group_path, O_RDWR); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    int ret = ioctl(fd, IOCTL_SET_SEND_DELAY, delay);

    printf("\n\nReturn code : %d\n\n", ret);

    return ret;
}

int revokeDelay(const char *group_path){

    int *fd;
    fd = open(group_path, O_RDWR); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    int ret = ioctl(fd, IOCTL_REVOKE_DELAYED_MESSAGES);

    printf("\n\nReturn code : %d\n\n", ret);

    return ret;
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



    char buffer[256];
    const unsigned int len = 50;
    int ret = read(fd, &buffer, sizeof(char)*len);

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

    close(fd);

}

void concurrentWrite(void *args){

    pthread_t id = pthread_self();
    printf("\n[w] Thread N. %ld\n", id);

    char buffer[256];


    strcpy(buffer, "PIPPO\0");
    int len = strlen(buffer);
    printf("\nLen = %d\n", len);


    int *fd;
    fd = open((char*)args, O_WRONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    int ret = write(fd, &buffer, sizeof(char)*len);

    printf("Totel element written: %ld", ret);


    //Random Sleep
    long sleep_time = rand()%10000;
    usleep(sleep_time);


    close(fd);

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

    int *fd;
    fd = open(group_path, O_RDONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    pause_char();

    char buffer[256];
    char string_buffer[256];
    unsigned int len = 60;

    int ret = read(fd, &buffer, sizeof(char)*len);

    printf("Read return value %d\n", ret);

    if(ret == 0){
        printf("[ ] No message is present\n");
    }else if(ret == MEMORY_ERROR){
        printf("[X] Memory Error\n");
    }else if(ret > 0){

        strncpy(string_buffer, buffer, ret);

        printf("Buffer content: %s\n", string_buffer);
    }


    pause_char();


    close(fd);

    return 0;

}


int writeGroup(const char *group_path){

    char buffer[256];

    strcpy(buffer, "PIPPO\0");

    int len = strlen(buffer);


    int *fd;
    fd = open(group_path, O_WRONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    pause_char();


    int ret = write(fd, &buffer, sizeof(char)*len);

    printf("Totel element written: %ld", ret);

    pause_char();

    close(fd);

    return;
}




int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s install main_device\n", argv[0]);
        printf("Usage: %s read group_device\n", argv[0]);
        printf("Usage: %s wr group_device\n", argv[0]);
        printf("Usage: %s cwr group_device\n", argv[0]);
        printf("Usage: %s delay group_device seconds\n", argv[0]);
        printf("Usage: %s flush group_device\n", argv[0]);
        printf("Usage: %s revoke group_device\n", argv[0]);

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

    }else if(strcmp(argv[1], "delay") == 0){

        long _delay = strtol(argv[3], NULL, 10);

        setDelay(argv[2], _delay);
    }else if(strcmp(argv[1], "flush") == 0){
        flushMessages(argv[2]);
    }else if(strcmp(argv[1], "revoke") == 0){
        revokeDelay(argv[2]);
    }else{
        printf("Unimplemented\n\n");
    }


    return 0;
}
