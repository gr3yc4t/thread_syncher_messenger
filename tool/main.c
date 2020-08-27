#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <sys/ioctl.h>

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)



typedef struct group_t {
	unsigned int group_id;		//Thread group ID
	char *group_name;
} group_t;


int installGroup(char *main_device_path){

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




int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Usage: %s install main_device\n", argv[0]);
        printf("Usage: %s open group_device\n", argv[0]);
        return -1;
    }


    if(strcmp(argv[1], "install") == 0){
        installGroup(argv[2]);
    }else{
        printf("Unimplemented\n\n");
    }


    return 0;
}
