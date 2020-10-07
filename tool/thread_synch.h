/**
 * @file thread_synch.h
 * 
 * @brief User-level library to wrap thread-synch kernel-level functionalities
 * 
 * Funcions will operate through two types of structures:
 *      - T_THREAD_SYNCH: handles request to the main synch device file
 *      - T_THREAD_GROUP: handles request to a group device file
 */


#ifndef THREAD_SYNCH_H
#define THREAD_SYNCH_H


#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif



#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

#define ATTR_BUFF_SIZE 64
#define DEVICE_NAME_SIZE    64      /**< Maximum device name lenght*/

#define GROUP_CLASS_NAME "group_synch"
#define CLASS_NAME		"thread_synch"				/**< Main device class name */

#define D_DEV_NAME		"main_thread_sync"			/**< Main device name */
#define D_DEV_MAJOR		(0)							/**< Main device major# */
#define D_DEV_MINOR		(0)							/**< Main device minor# */


#define GRP_MIN_ID 		0							/**< Group's min ID */
#define GRP_MAX_ID		255							/**< Group's max ID */


/** @brief Errors code*/
#define NO_MSG_PRESENT       1

#define ALLOC_ERR 			-2	 
#define USER_COPY_ERR		-3
#define SEM_INIT_ERR        -4  /** @brief Semaphore initialization **/
#define MSG_INVALID_FORMAT  -11
#define MSG_SIZE_ERROR      -12
#define MEMORY_ERROR        -13
#define STORAGE_SIZE_ERR    -14




#define TS_NOT_FOUND    -50
#define TS_OPEN_ERR     -51
#define UNAUTHORIZED    -52
#define PERMISSION_ERR  -53




#define BUFF_SIZE 512

/**
 * @brief ioctls 
 */

#define IOCTL_GET_GROUP_DESC _IOR('Q', 1, group_t*)


#define IOCTL_SET_SEND_DELAY _IOW('Y', 0, long)
#define IOCTL_REVOKE_DELAYED_MESSAGES _IO('Y', 1)

#define IOCTL_SLEEP_ON_BARRIER _IO('Z', 0)
#define IOCTL_AWAKE_BARRIER _IO('Z', 1)

#define IOCTL_INSTALL_GROUP _IOW('X', 99, group_t*)
#define IOCTL_GET_GROUP_ID  _IOW('X', 100, group_t*)

#define IOCTL_CHANGE_OWNER _IOW('Q', 102, uid_t)
#define IOCTL_SET_STRICT_MODE _IOW('Q', 101, bool)


/**
 * @brief Basic structure to represent a message
 */
typedef struct t_message{
    pid_t author;           /**< Process which wrote the message*/
    void *buffer;           /**< Buffer containing the message*/
    size_t size;            /**< Size (in bytes) of the buffer*/
} msg_t;

/**
 * @brief System-wide descriptor of a group
 */
typedef struct group_t {
	char *group_name;           
    ssize_t name_len;
} group_t;



/**
 * @brief User-level handler of the thread-synch main device
 */
typedef struct T_THREAD_SYNCH {
    int main_file_descriptor;   /**< File descriptor of the /dev/main_synch device */

    char *main_device_path;     /**< Path to the main syncher file inside /dev directory*/
    size_t path_len;            /**< Length of the main syncher path */

    short initialized;          /**< Flag that specifies if the structure is initialized */
} thread_synch_t;


/**
 * @brief User-level handler of a group
 */
typedef struct T_THREAD_GROUP {

    int file_descriptor;    /**< File descriptor of the /dev device */

    group_t descriptor;     /**< System-wide descriptor of the group */
    unsigned int group_id;  /**< System-wide group ID */
    
    char *group_path;       /**< Path to the group file inside /dev directory*/
    size_t path_len;        /**< Length of the group path */

}  thread_group_t;



static const char *param_default_path = "/sys/class/group_synch/synch!group%d/group_parameters/";
static const char *group_default_path = "/dev/synch/group%d";



int initThreadSyncher(thread_synch_t *main_syncher);
thread_group_t* installGroup(const group_t group_descriptor, thread_synch_t *main_synch);

int readGroupInfo(thread_synch_t *main_syncher);


int openGroup(thread_group_t* group);
int readMessage(void *buffer, size_t len, thread_group_t *group);
int writeMessage(const void *buffer, size_t len, thread_group_t *group);

int setDelay(const long _delay, thread_group_t *group);
int revokeDelay(thread_group_t *group);

int sleepOnBarrier(thread_group_t *group);
int awakeBarrier(thread_group_t *group);

unsigned long getCurrentStorageSize(thread_group_t *group);
unsigned long getMaxStorageSize(thread_group_t *group);
unsigned long getMaxMessageSize(thread_group_t *group);

int setMaxMessageSize(thread_group_t *group, unsigned long val);
int setMaxStorageSize(thread_group_t *group, unsigned long val);
int setGarbageCollectorRatio(thread_group_t *group, unsigned long val);


int enableStrictMode(thread_group_t *group);
int disableStrictMode(thread_group_t *group);
int changeOwner(thread_group_t *group, const uid_t new_owner);
int becomeOwner(thread_group_t *group);



thread_group_t *loadGroupFromDescriptor(const group_t *descriptor, thread_synch_t *main_syncher);
thread_group_t* loadGroupFromID(const int group_id);

#endif  //THREAD_SYNCH_H