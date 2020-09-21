#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> //For sleep()/nanosleep()/getppid()
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>


//IOCTL List
#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)
#define IOCTL_SET_SEND_DELAY _IOW('Y', 0, long)
#define IOCTL_REVOKE_DELAYED_MESSAGES _IO('Y', 1)
#define IOCTL_SLEEP_ON_BARRIER _IO('Z', 0)
#define IOCTL_AWAKE_BARRIER _IO('Z', 1)

//Configurations
#define THREAD_NUM 64
#define BUFF_SIZE 512


//Error Codes
#define NO_MSG_PRESENT -10
#define MSG_INVALID_FORMAT -11
#define MSG_SIZE_ERROR  -12
#define MEMORY_ERROR -13


/**
 * @brief System-wide descriptor of a group
 */
typedef struct group_t {
	char *group_name;           //TODO: add name len.
    size_t name_len;
} group_t;



//Global var describing threads
pthread_t tid[THREAD_NUM];


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


int openGroup(const char *group_path){
    int *fd;
    fd = open(group_path, O_RDWR); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    return fd;
}

int _readGroup(int fd){
    char buffer[256];
    char string_buffer[256];
    unsigned int len = 60;

    int ret = read(fd, &buffer, sizeof(char)*len);

    printf("Read return value %d\n", ret);

    if(ret == 0){
        printf("[ ] No message is present\n");
        return 0;
    }else if(ret == MEMORY_ERROR){
        printf("[X] Memory Error\n");
        return -1;
    }else if(ret > 0){

        strncpy(string_buffer, buffer, ret);

        printf("Buffer content: %s\n", string_buffer);
        return 1;
    }

    return -1;
}

int _writeGroup(int fd, char *buffer, ssize_t len){
    return write(fd, buffer, sizeof(char)*len);
}

int _setDelay(int fd, const long delay){
    return ioctl(fd, IOCTL_SET_SEND_DELAY, delay);
}

int _revokeDelay(int fd){
    return ioctl(fd, IOCTL_REVOKE_DELAYED_MESSAGES);
}

int _sleepOnBarrier(int fd){
    return ioctl(fd, IOCTL_SLEEP_ON_BARRIER);
}


int _setMaxMsgSize(unsigned long _val){

    int *fd;
    int ret;
    fd = open("/sys/class/group_synch/group0/message_param/max_message_size", O_WRONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    const char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("Error while converting the paramtere value");
        return -1;
    }

    ret = write(fd, buff, sizeof(char)*strlen(buff));

    return ret;
}

int _setMaxStorageSize(unsigned long _val){

    int *fd;
    int ret;
    fd = open("/sys/class/group_synch/group0/message_param/max_storage_size", O_WRONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    const char buff[BUFF_SIZE];

    if(sprintf(buff, "%ld", _val) < 0){
        printf("Error while converting the paramtere value");
        return -1;
    }

    ret = write(fd, buff, sizeof(char)*strlen(buff));

    return ret;
}

int _setGarbCollRatio(unsigned int _val){

    int *fd;
    int ret;
    fd = open("/sys/class/group_synch/group0/message_param/garbage_collector_ratio", O_WRONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }


    const char buff[BUFF_SIZE];

    if(sprintf(buff, "%ud", _val) < 0){
        printf("Error while converting the paramtere value");
        return -1;
    }

    ret = write(fd, buff, sizeof(char)*strlen(buff));

    return ret;
}


int _readMaxMsgSize(unsigned long *_val){
    FILE *fd;
    int ret;

    fd = fopen("/sys/class/group_synch/group0/message_param/max_message_size", "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    char buff[BUFF_SIZE];

    printf("Reading param 'max_message_size'");
    ret = fread(buff, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parametersaaa: %d", ret);
        return -1;
    }

    *_val = strtoul(buff, NULL, 10);

    fclose(fd);

    return 0;
}

int _readMaxStorageSize(unsigned long *_val){
    FILE *fd;
    int ret;

    fd = fopen("/sys/class/group_synch/group0/message_param/max_storage_size", "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    char buff[BUFF_SIZE];

    printf("Reading param 'max_storage_size'");
    ret = fread(buff, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parameters: %d", ret);
        return -1;
    }



    *_val = strtoul(buff, NULL, 10);

    fclose(fd);

    return 0;
}

int _readCurrStorageSize(unsigned long *_val){
    FILE *fd;
    int ret;

    fd = fopen("/sys/class/group_synch/group0/message_param/current_message_size", "rt"); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    char buff[BUFF_SIZE];

    printf("Reading param 'curr_storage_size'");
    ret = fread(buff, sizeof(char), BUFF_SIZE, fd);
    if(ret < 0){
        printf("Errror while reading parameters: %d", ret);
        return -1;
    }

    *_val = strtoul(buff, NULL, 10);

    fclose(fd);

    return 0;
}




int printGroupParams(){
    int ret;

    unsigned long max_message_size = 0;
    unsigned long max_storage_size = 0;
    unsigned long curr_storage_size = 0;


    if(_readMaxMsgSize(&max_message_size) < 0)
        return -1;
    
    if(_readMaxStorageSize(&max_storage_size) < 0)
        return -1;
    
    if(_readCurrStorageSize(&curr_storage_size) < 0)
        return -1;
    
    printf("\nGroup 0 Data\n\n\tMax message size:  %ld\n\tMax Storage Size: %ld\n\tCurrent storage size: %ld\n\n", max_message_size, max_storage_size, curr_storage_size);

    return 0;
}



int _awakeBarrier(int fd){
    return ioctl(fd, IOCTL_AWAKE_BARRIER);
}

int sleepOnBarrier(const char *group_path){

    int fd = openGroup(group_path);

    int ret = ioctl(fd, IOCTL_SLEEP_ON_BARRIER);

    printf("\n\nReturn code : %d\n\n", ret);

    return ret;
}

int awakeBarrier(const char *group_path){

    int *fd;
    fd = open(group_path, O_RDWR); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    int ret = ioctl(fd, IOCTL_AWAKE_BARRIER);

    printf("\n\nReturn code : %d\n\n", ret);

    return ret;
}

int setDelay(const char *group_path, const long delay){

    int *fd;
    fd = open(group_path, O_RDWR); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    int ret = _setDelay(fd, delay);

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

    int ret = _revokeDelay(fd);

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

    new_group.group_name = "pippo";
    new_group.name_len = strlen(new_group.group_name);

    int ret = ioctl(numeric_descriptor, IOCTL_INSTALL_GROUP, &new_group);

    printf("\n\nGroup ID : %d\n\n", ret);


    fclose(fd);

    return ret;
}

int readGroup(const char *group_path){

    int *fd;
    fd = open(group_path, O_RDONLY); 

    if(fd < 0){
        printf("Error while opening the group file\n");
        return -1;
    }

    pause_char();

    _readGroup(fd);

    fflush(stdin);

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

    int ret = _writeGroup(fd, &buffer, sizeof(char)*len);

    printf("Totel element written: %ld", ret);

    fflush(stdin);

    pause_char();

    close(fd);

    return;
}


int interactiveSession(const char *group_path){

    int choice;
    int exit_flag = 0;
    char *buffer;
    int ret = 99;
    long delay;
    long msg_size;
    long storage_size;
    char buff_size[64];
    int ratio;


    pid_t my_pid = getppid();

    int fd = openGroup(group_path);

    if(fd == -1){
        printf("\n[X] Unable to open group, exiting...");
        return -1;
    }

    do{
    printf("\n[Group Management] - %d\n", my_pid);
    printf("Select Options:\n\t1 - Read\n\t2 - Write\n\t3 - Set Delay\n\t"
        "4 - Revoke Delay\n\t5 - Flush\n\t6 - Sleep on Barrier\n\t7 - Awake barrier"
        "\n -Message Param -\n\t 80 - Show Message Param \n\t81 - Set max message size\n\t"
        "82 - Set max storage size\n\t83 - Set Garbage Collector ratio\n\t"
        "99 - Exit\n:");

    scanf(" %d", &choice);

    clear();


        switch (choice){
        case 1:     //Read
            _readGroup(fd);
            break;
        case 2:     //Write

            buffer = (char*)malloc(sizeof(BUFF_SIZE));
            if(!buffer)
                return;

            printf("\nContent: ");
            if(scanf(" %s", buffer) > BUFF_SIZE)
                return;
            
            int len = strlen(buffer);
            
            ret = _writeGroup(fd, buffer, (ssize_t)len);

            free(buffer);
            break;
        case 3: //Set Delay
            printf("\nDelay Value: ");
            char buff_delay[64];
            scanf("%s", buff_delay);

            delay = strtol(buff_delay, NULL, 10);

            ret = _setDelay(fd, delay);
            break;
        case 4: //Set Delay
            ret = _revokeDelay(fd);
            printf("\nDelayed message revoked");
            break;
        case 5: //Flush
            printf("\nUnimplemented");
            break;
        case 6: //Sleep on barrier
            printf("\nThread is going to sleep...");
            ret = _sleepOnBarrier(fd);
            printf("\nThread awaken!!!");
            break;
        case 7: //Awake barrier
            ret = _awakeBarrier(fd);
            printf("\nBarrier Awaked!");
            break;
        case 80:
            if(printGroupParams() < 0)
                printf("Error while reading parameters");
            break;
        case 81:
            printf("\nMax size value: ");

            scanf("%s", buff_size);

            msg_size = strtol(buff_size, NULL, 10);

            ret = _setMaxMsgSize(msg_size);
            break;
        case 82:
            printf("\nMax storage value: ");

            scanf("%s", buff_size);

            storage_size = strtol(buff_size, NULL, 10);

            ret = _setMaxStorageSize(storage_size);   
            break;
        case 83:
            printf("\nGarbage collector ratio: ");

            scanf("%d", &ratio);
            ret = _setGarbCollRatio(ratio);

            break;        
        case 99:
            printf("\n\nExiting...\n");
            exit_flag = 1;
            break;
        default:
            printf("\n\nInvalid command\n\n");
            break;
        }

        printf("\nReturn Value: %d\n", ret);

    }while(exit_flag == 0);
}


int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s interactive group_device\n", argv[0]);

        printf("Usage: %s install main_device\n", argv[0]);
        printf("Usage: %s read group_device\n", argv[0]);
        printf("Usage: %s wr group_device\n", argv[0]);
        printf("Usage: %s cwr group_device\n", argv[0]);
        printf("Usage: %s delay group_device seconds\n", argv[0]);
        printf("Usage: %s flush group_device\n", argv[0]);
        printf("Usage: %s revoke group_device\n", argv[0]);
        printf("Usage: %s sleep group_device\n", argv[0]);
        printf("Usage: %s awake group_device\n", argv[0]);
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
    }else if(strcmp(argv[1], "sleep") == 0){
        sleepOnBarrier(argv[2]);
    }else if(strcmp(argv[1], "awake") == 0){
        awakeBarrier(argv[2]);
    }else if(strcmp(argv[1], "interactive") == 0){
        interactiveSession(argv[2]);
    }else{
        printf("Unimplemented\n\n");
    }


    return 0;
}
