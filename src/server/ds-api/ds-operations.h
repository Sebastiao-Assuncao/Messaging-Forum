#ifndef DS_OPERATIONS_H
#define DS_OPERATIONS_H

#include "../../centralizedmsg-api.h"
#include "../../centralizedmsg-api-constants.h"

/* Struct that mantains information about each group in the DS */
typedef struct ginfo
{
    char no[DS_GID_SIZE];
    char name[DS_GNAME_SIZE];
} GroupInfo;

/* Struct that maintains information about all groups in the DS */
typedef struct glist
{
    GroupInfo groupinfo[DS_MAX_NUM_GROUPS];
    int no_groups;
} GroupList;

/* Variable that is used to keep all information about the DS's groups */
extern GroupList dsGroups;

/**
 * @brief Fills the dsGroups struct variable with all the existing groups in the beggining of the program.
 *
 */
void fillDSGroupsInfo();

/**
 * @brief Checks if a given path is a directory.
 *
 * @param path string that contains a path.
 * @return 1 if it's a directory, 0 otherwise.
 */
int directoryExists(const char *path);

/**
 * @brief Checks if a given DS client password matches the stored one.
 *
 * @param clientID string that contains the ID of the client.
 * @param clientPW string that contains the password to be compared to the stored one.
 * @return 1 if they match, 0 otherwise.
 */
int passwordsMatch(const char *userID, const char *userPW);

/**
 * @brief Unsubscribes a given user ID from all of its subscribed groups.
 *
 * @param userID string that contains the ID of the client.
 * @return 1 if user was unsubscribed from all groups, 0 otherwise.
 */
int unsubscribeClientFromGroups(const char *userID);

/**
 * @brief Deletes a directory from the DS.
 *
 * @param path string containing the directory to be deleted.
 * @return 1 if the directory was deleted, 0 otherwise.
 */
int removeDirectory(const char *path);

/**
 * @brief Puts in a given buffer a message containing all desired DS groups.
 *
 * @param buffer string that will contain the DS groups.
 * @param groups if NULL add to buffer all DS groups, otherwise add to buffer all client subscribed groups.
 * @param num if 0 then add to buffer all DS groups, otherwise add to buffer all client subscribed groups.
 * @return 1 if buffer contains all desired DS groups, 0 otherwise.
 */
int createGroupListMessage(char *buffer, int *groups, int num);

/**
 * @brief Checks if the given GName of a GID is equal to the stored one.
 *
 * @param GID string that contains the ID of the group being check.
 * @param GName string that contains the name being compared to the stored one.
 * @return 1 if the group names match, 0 otherwise.
 */
int groupNamesMatch(const char *GID, const char *GName);

/**
 * @brief Fills an integer array with the groups that the user with ID UID is subscribed to.
 *
 * @param UID string that contains the client ID.
 * @param clientGroupsSubscribed array that will be filled with the groups.
 * @param numGroupsSub reference to integer containing number of groups that client is subscribed to.
 * @return 1 if the array was properly filled, 0 otherwise.
 */
int fillClientSubscribedGroups(const char *UID, int *clientGroupsSubscribed, int *numGroupsSub);

/**
 * @brief Buffers all users in a given DS group.
 *
 * @param GID string that contains the group ID.
 * @return buffer that contains all the users subscribed to the group with ID GID.
 */
char *listUsersInDSGroup(const char *GID);

/**
 * @brief Checks if a given client is subscribed to a given DS group.
 *
 * @param UID string that contains the client's UID.
 * @param GID string that contains the DS's GID.
 * @return 1 if client is subscribed to group, 0 otherwise.
 */
int userSubscribedToGroup(const char *UID, const char *GID);

/**
 * @brief Creates a new message in a group.
 *
 * @param newMID string that will contain the new message ID.
 * @param UID string that contains the author of the message.
 * @param GID string that contais the group ID where the message was sent.
 * @param TSize integer than contains the message text size in bytes.
 * @param Text string that contains the message text.
 * @return 1 if the message was successfully posted, 0 otherwise.
 */
int createMessageInGroup(char *newMID, const char *UID, const char *GID, int TSize, const char *Text);

/**
 * @brief Checks the number of messages to retrieve.
 *
 * @param GID string that contais the group ID where messages will be retrieved from.
 * @param MID string that contains the starting message ID to retrieve.
 * @return number of messages to retrieve N (0 <= N <= 20) if no errors, -1 otherwise.
 */
int checkNumberOfMsgsToRet(const char *GID, int MID);

/**
 * @brief Retrieves N (1 <= N <= 20) messages from a given DS group.
 *
 * @param fd file descriptor where the TCP connection was made to request this command.
 * @param startMID integer that contains the starting message.
 * @param numMsgsToRet integer that contains N.
 * @return 1 if retrieve was successful, 0 otherwise.
 */
int retrieveDSGroupMessages(int fd, const char *GID, int startMID, int numMsgsToRet);

#endif