#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)


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

    char buffer[256];


    int ret = fread(buffer, sizeof(char), sizeof(char)*256, fd);


    printf("Buffer content: %s", buffer);

    return 0;

}


int writeGroup(const char *group_path){

    char buffer[256];

    strcpy(buffer, "PIPPO\0");

    int len = strlen(buffer);

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


    printf("Bytes written: %ld", ret);
    printf("\nsizeof(msg_t) = %ld", sizeof(msg_t));

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
    }else{
        printf("Unimplemented\n\n");
    }


    return 0;
}
