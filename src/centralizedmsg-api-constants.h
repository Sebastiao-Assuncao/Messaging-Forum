#ifndef CENTRALIZEDMESSAGING_API_CONSTANTS_H
#define CENTRALIZEDMESSAGING_API_CONSTANTS_H

/* Preprocessed macro to determine min(x,y) */
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* DS wrong protocol message */
#define ERR_MSG "ERR\n"

/* DS wrong protocol message bytes */
#define ERR_MSG_SIZE 4

/* By default DS server is set to be listening on localhost */
#define DS_DEFAULT_ADDR "127.0.0.1"

/* By default DS server is set to be listening on port 58000 + GN */
#define DS_DEFAULT_PORT "58018"

/* Hostnames (including the dots) can be at most 253 characters long */
#define DS_ADDR_SIZE 254

/* Ports range from 0 to 65535 */
#define DS_PORT_SIZE 6

/* The maximum buffer size to read from stdin from client side (it's actually around 273 but we'll give a seg fault margin for wrong input) */
#define CLIENT_COMMAND_SIZE 512

/* The maximum number of words written on command in stdin by the client (it's actually around 256 but we'll give a seg fault margin too) */
#define CLIENT_NUMTOKENS 512

/* Macros used to parse client command and use switch cases instead of strcmp */
#define INVALID_COMMAND -1
#define REGISTER 1
#define UNREGISTER 2
#define LOGIN 3
#define LOGOUT 4
#define SHOWUID 5
#define EXIT 6
#define GROUPS 7
#define SUBSCRIBE 8
#define UNSUBSCRIBE 9
#define MY_GROUPS 10
#define SELECT 11
#define SHOWGID 12
#define ULIST 13
#define POST 14
#define RETRIEVE 15

/* The buffer size for a message from the client to the DS via UDP protocol */
#define CLIENT_TO_DS_UDP_SIZE 40

/* The buffer size for a message from the DS to the client via UDP protocol */
#define DS_TO_CLIENT_UDP_SIZE 4096

/* The buffer size for a protocol message code */
#define PROTOCOL_CODE_SIZE 4

/* The buffer size for a protocol message status via UDP protocol */
#define PROTOCOL_STATUS_UDP_SIZE 8

/* Macros used to keep current client session state */
#define LOGGED_OUT 0
#define LOGGED_IN 1

/* The size of a client's UID according to the statement's rules */
#define CLIENT_UID_SIZE 6

/* The size of a client's password according to the statement's rules */
#define CLIENT_PWD_SIZE 9

/* The size of a buffer containing all relevant information about a DS group */
#define DS_GROUPINFO_SIZE 34

/* The size of a DS's group ID according to the statement's rules */
#define DS_GID_SIZE 3

/* The maximum size of DS group name */
#define DS_GNAME_SIZE 25

/* The size of a DS group message ID according to the statement's rules */
#define DS_MID_SIZE 5

/* The size of a ulist command buffer from the client to the server */
#define CLIENTDS_ULISTBUF_SIZE 8

/* Arbitrary size to read from TCP in ULIST each time readTCP is called */
#define DSCLIENT_ULISTREAD_SIZE 512

/* The buffer size for a protocol message status via UDP protocol */
#define PROTOCOL_STATUS_TCP_SIZE 5

/* The maximum size of a buffer containing the length of a text message */
#define PROTOCOL_TEXTSZ_SIZE 4

/* The maximum number of ASCII characters in a text message */
#define PROTOCOL_TEXT_SIZE 241

/* The maximum size of a buffer that contains the file name of the file being uploaded on post */
#define PROTOCOL_FNAME_SIZE 25

/* The size of a post command buffer with file excluding the file data that the client will send to the DS */
#define CLIENTDS_POSTWFILE_SIZE 295

/* The size of a post command buffer without a file included that will be sent to the DS */
#define CLIENTDS_POSTWOFILE_SIZE 259

/* The size of a post command reply buffer from the DS to the client */
#define DS_POSTREPLY_SIZE 10

/* The size of the unsigned char buffer that is read from the file and sent to a fd via TCP protocol */
#define FILEBUFFER_SIZE 2048

/* The size of a retrieve command buffer from the client to the DS */
#define CLIENTDS_RTVBUF_SIZE 19

/* The size of a retrieve status code from the DS to the client */
#define DSCLIENT_RTVSTATUS_SIZE 3

/* The size of a buffer that contains a single char from a DS reply */
#define CHAR_SIZE 2

/* The size of a buffer that contains the number of messages to retrieve */
#define DSCLIENT_RTVNMSG_SIZE 2

/* Macros used to keep retrieve function informed if first character of a MID has been read or not */
#define MID_OK 0     // no need to concatenate
#define MID_CONCAT 1 // need to concatenate -> first char of MID was previously read

/* The size of a buffer that contains the number of bytes in a group message text */
#define DS_MSGTEXTSZ_SIZE 4

/* The size of a buffer that contains the number of bytes in a group message file */
#define PROTOCOL_FILESZ_SIZE 11

/* Macros for verbose on the DS */
#define VERBOSE_OFF 0
#define VERBOSE_ON 1

/* Default size for the DS TCP listen queue */
#define DS_LISTENQUEUE_SIZE 10

/* Default DS hostname buffer size */
#define DS_HOSTNAME_SIZE 1023

/* Maximum number of existing groups in the DS */
#define DS_MAX_NUM_GROUPS 100

/* Macro used to read d_name attribute from struct dirent in all of DS operations */
#define DIRENT_NAME_SIZE 256

/* The size of a buffer containing a DS group's name file */
#define DS_GNAMEPATH_SIZE 32

/* The size of a buffer containing a registered DS client folder */
#define DS_CLIENTDIRPATH_SIZE 19

/* The size of a buffer containing a registered DS client password file */
#define DS_CLIENTPWDPATH_SIZE 36

/* The size of a buffer containing a user subscribed to group file path */
#define DS_GROUPCLIENTSUBPATH_SIZE 27

/* The size of a buffer containing a registered DS client login file */
#define DS_CLIENTLOGINPATH_SIZE 37

/* The size of a buffer containing a path to a DS group MSG folder */
#define DS_GROUPMSGPATH_SIZE 21

/* The size of a buffer containing all groups in the DS */
#define DS_GROUPSLISTBUF_SIZE 3270

/* The size of a buffer containing each group information */
#define DS_GROUPINFOBUF_SIZE 34

/* The size of a buffer containing a DS group's folder path */
#define DS_GROUPDIRPATH_SIZE 17

/* The size of a buffer containing a new DS groups status message */
#define DS_NEWGROUPSTATUS_SIZE 7

/* The size of a buffer containing a DS message to the client to send the command status */
#define DS_TCPSTATUSBUF_SIZE 32

/* The initial size for a dynamically allocated buffer that contains all users in a DS group */
#define DS_ULISTBUFINIT_SIZE 302 // 50 users

/* The size of a buffer containing a group message directory path */
#define DS_GROUPMSGDIRPATH_SIZE 27

/* The size of a buffer containing the path to a message's author file */
#define DS_GROUPMSGAUTHORPATH_SIZE 43

/* The size of a buffer containing the path to a message's text file */
#define DS_GROUPMSGTEXTPATH_SIZE 39

/* The size of a buffer containing the path to a message file */
#define DS_GROUPMSGFILEPATH_SIZE 53

/* The size of a buffer containing the initial retrieve status message */
#define DS_RETINITSTATUS_SIZE 6

/* Macros for file in message directory flag */
#define NO_FILE 0
#define HAS_FILE 1

/* The size of bufferS that contain fragments of message information to be retrieved from the DS to the client */
#define DS_MSGTEXTINFO_SIZE 257
#define DS_MSGFILEINFO_SIZE 41

/* The size of a buffer to receive confirmation from the DS to the client */
#define DS_RETCONFBUF_SIZE 256

/* The default number of tries to recover packets that were sent via UDP protocol */
#define DEFAULT_UDPRECV_TRIES 3

#endif