#include "ds-operations.h"
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

GroupList dsGroups;

void fillDSGroupsInfo()
{
    DIR *d;
    struct dirent *dir;
    int i = 0;
    FILE *fp;
    char groupID[DS_GID_SIZE];
    char groupNamePath[DS_GNAMEPATH_SIZE];
    (&dsGroups)->no_groups = 0;
    d = opendir("server/GROUPS");
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
                continue;
            if (strlen(dir->d_name) != 2)
                continue;
            // If it gets here then we copy the first 2 letters of the directory name
            strncpy(groupID, dir->d_name, DS_GID_SIZE - 1);
            groupID[DS_GID_SIZE - 1] = '\0';

            // Fill the global ds struct
            strcpy((&dsGroups)->groupinfo[i].no, groupID);

            // Open the group name file and fill the global ds struct
            sprintf(groupNamePath, "server/GROUPS/%s/%s_name.txt", groupID, groupID);
            fp = fopen(groupNamePath, "r");
            if (fp)
            {
                fscanf(fp, "%24s", (&dsGroups)->groupinfo[i].name);
                fclose(fp);
            }
            ++i;
            if (i == 99)
            {
                break;
            }
        }
        (&dsGroups)->no_groups = i;
        closedir(d);
    }
}

int directoryExists(const char *path)
{
    struct stat stats;
    if (stat(path, &stats))
    {
        return 0;
    }
    return S_ISDIR(stats.st_mode);
}

int passwordsMatch(const char *userID, const char *userPW)
{
    char clientPwdPath[DS_CLIENTPWDPATH_SIZE];
    char clientStoredPwd[CLIENT_PWD_SIZE];
    size_t n;

    // Open stored password file
    sprintf(clientPwdPath, "server/USERS/%s/%s_pass.txt", userID, userID);
    FILE *pwd = fopen(clientPwdPath, "r");
    if (pwd == NULL)
    {
        return 0;
    }

    // Read password from stored file
    n = fread(clientStoredPwd, sizeof(char), CLIENT_PWD_SIZE - 1, pwd);
    if (n == -1)
    {
        return 0;
    }

    // Close stored file
    if (fclose(pwd) == -1)
    {
        return 0;
    }

    // Compare stored and given passwords
    if (strcmp(clientStoredPwd, userPW))
    {
        return 0;
    }

    return 1;
}

int unsubscribeClientFromGroups(const char *userID)
{
    DIR *d;
    struct dirent *dir;
    char groupID[DS_GID_SIZE];
    char userSubscribedFile[DS_GROUPCLIENTSUBPATH_SIZE];
    d = opendir("server/GROUPS");
    if (d == NULL)
    {
        return 0;
    }
    while ((dir = readdir(d)) != NULL)
    {
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
        {
            continue;
        }
        strncpy(groupID, dir->d_name, DS_GID_SIZE - 1);
        groupID[DS_GID_SIZE - 1] = '\0';
        sprintf(userSubscribedFile, "GROUPS/%s/%s.txt", groupID, userID);
        if (!access(userSubscribedFile, F_OK))
        { // User is subscribed to this group
            if (unlink(userSubscribedFile) == -1)
            { // Delete its file
                return 0;
            }
        }
    }
    if (closedir(d) == -1)
    {
        return 0;
    }
    return 1;
}

int removeDirectory(const char *path)
{
    size_t pathLen;
    char *fullPath;
    DIR *dir;
    struct stat statPath, statEntry;
    struct dirent *entry;

    // Stat for the given path
    stat(path, &statPath);

    // Check if path is a dir
    if (!S_ISDIR(statPath.st_mode))
    {
        return 0;
    }

    // Open directory
    if ((dir = opendir(path)) == NULL)
    {
        return 0;
    }

    // Determine length of the directory path
    pathLen = strlen(path);

    // Iterate over each entry in directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Ignore "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // Determina a full path of an entry
        fullPath = calloc(sizeof(char), pathLen + strlen(entry->d_name) + 1);
        if (fullPath == NULL)
        {
            return 0;
        }
        strcpy(fullPath, path);
        strcat(fullPath, "/");
        strcat(fullPath, entry->d_name);

        // Stat for the full path
        stat(fullPath, &statEntry);

        // Recursively remove a nested directory
        if (S_ISDIR(statEntry.st_mode))
        {
            removeDirectory(fullPath);
            continue;
        }

        // Remove a file object
        if (unlink(fullPath) == -1)
        {
            return 0;
        }
        free(fullPath);
    }

    // Remove empty directory
    if (rmdir(path) == -1)
    {
        return 0;
    }

    // Close the directory
    if (closedir(dir) == -1)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Compares two DS groups by their GID.
 *
 * @param a a DS group.
 * @param b another DS group.
 * @return result of strcmp between a and b.
 */
static int compare(const void *a, const void *b)
{
    GroupInfo *q1 = (GroupInfo *)a;
    GroupInfo *q2 = (GroupInfo *)b;
    return strcmp(q1->no, q2->no);
}

/**
 * @brief Sorts a GroupList struct by their groups' IDs.
 *
 * @param list GroupList struct to be sorted.
 */
static void sortGList(GroupList *list)
{
    qsort(list->groupinfo, list->no_groups, sizeof(GroupInfo), compare);
}

int createGroupListMessage(char *buffer, int *groups, int num)
{
    char infoDSGroup[DS_GROUPINFO_SIZE];
    fillDSGroupsInfo(); // In case manual directories were inputted during program execution
    int numDSGroups = (groups == NULL) ? dsGroups.no_groups : num;
    sortGList((&dsGroups));
    sprintf(infoDSGroup, "%d", numDSGroups);
    strcat(buffer, infoDSGroup);
    if (numDSGroups > 0)
    {
        struct dirent **d;
        int n, flag;
        int j;
        char dsGroupMsgPath[DS_GROUPMSGPATH_SIZE];
        for (int i = 0; i < numDSGroups; ++i)
        {
            j = (groups == NULL) ? i : groups[i];
            sprintf(dsGroupMsgPath, "server/GROUPS/%s/MSG", dsGroups.groupinfo[j].no);
            n = scandir(dsGroupMsgPath, &d, 0, alphasort);
            if (n < 0)
            {
                return 0;
            }
            else
            {
                flag = 0;
                while (n--)
                {
                    if (!flag)
                    {
                        if (validMID(d[n]->d_name))
                        { // Check for garbage
                            char MID[DS_MID_SIZE];
                            strncpy(MID, d[n]->d_name, DS_MID_SIZE - 1);
                            MID[DS_MID_SIZE - 1] = '\0';
                            sprintf(infoDSGroup, " %s %s %s", dsGroups.groupinfo[j].no, dsGroups.groupinfo[j].name, MID);
                            strcat(buffer, infoDSGroup);
                            flag = 1;
                        }
                    }
                    free(d[n]);
                }
                free(d);
                if (!flag)
                { // No messages are in the group
                    sprintf(infoDSGroup, " %s %s 0000", dsGroups.groupinfo[j].no, dsGroups.groupinfo[j].name);
                    strcat(buffer, infoDSGroup);
                }
            }
        }
    }
    return 1;
}

int groupNamesMatch(const char *GID, const char *GName)
{
    // Open group name file
    char groupNamePath[DS_GNAMEPATH_SIZE];
    sprintf(groupNamePath, "server/GROUPS/%s/%s_name.txt", GID, GID);
    FILE *name = fopen(groupNamePath, "r");
    if (name == NULL)
    {
        return 0;
    }
    fseek(name, 0, SEEK_END);
    int length = ftell(name) - 1; // -1 to avoid \n
    fseek(name, 0, SEEK_SET);
    char *realGName = calloc(sizeof(char), length + 1);
    if (!realGName)
    {
        return 0;
    }

    // Read group name
    size_t n = fread(realGName, 1, length, name);
    if (n == -1)
    {
        fprintf(stderr, "[-] Unable to read from group name file.\n");
        return 0;
    }
    realGName[length] = '\0';
    if (fclose(name) == -1)
    {
        return 0;
    }

    // Compare group names
    if (strcmp(realGName, GName) != 0)
    { // Group names don't match
        free(realGName);
        return 0;
    }
    // Group names match
    free(realGName);
    return 1;
}

/**
 * @brief Compares two group IDS (in integer).
 *
 * @param a a DS GID.
 * @param b another DS GID.
 * @return result of the difference between a and b.
 */
int compareIDs(const void *a, const void *b)
{
    return *(int *)a - *(int *)b;
}

int fillClientSubscribedGroups(const char *UID, int *clientGroupsSubscribed, int *numGroupsSub)
{
    DIR *d;
    struct dirent *dir;
    char GID[DS_GID_SIZE];
    char dsGroupClientSubPath[DS_GROUPCLIENTSUBPATH_SIZE];
    *numGroupsSub = 0;
    d = opendir("server/GROUPS");
    if (d == NULL)
    {
        return 0;
    }
    while ((dir = readdir(d)) != NULL)
    {
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
        {
            continue;
        }
        if (strlen(dir->d_name) != 2)
        {
            continue;
        }
        strncpy(GID, dir->d_name, DS_GID_SIZE - 1);
        GID[DS_GID_SIZE - 1] = '\0';
        sprintf(dsGroupClientSubPath, "server/GROUPS/%s/%s.txt", GID, UID);
        if (!access(dsGroupClientSubPath, F_OK))
        { // User is subscribed to this group -> save its index
            clientGroupsSubscribed[(*numGroupsSub)++] = atoi(GID) - 1;
        }
    }
    if (closedir(d) == -1)
    {
        return 0;
    }
    if (*numGroupsSub > 0)
    { // sort group index's
        qsort(clientGroupsSubscribed, *numGroupsSub, sizeof(int), compareIDs);
    }
    return 1;
}

char *listUsersInDSGroup(const char *GID)
{
    char dsGroupPath[DS_GROUPDIRPATH_SIZE];
    sprintf(dsGroupPath, "server/GROUPS/%s", GID);
    DIR *d;
    // Open the group directory
    d = opendir(dsGroupPath);
    if (d == NULL)
    {
        return NULL;
    }
    struct dirent *dir;
    char UID[CLIENT_UID_SIZE];
    char *users = (char *)calloc(sizeof(char), DS_ULISTBUFINIT_SIZE);
    if (users == NULL)
    { // Failed to allocate memory
        return NULL;
    }
    size_t lenUsers = DS_ULISTBUFINIT_SIZE;
    int cur = 0;
    while ((dir = readdir(d)) != NULL)
    {
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
        {
            continue;
        }
        if (strlen(dir->d_name) != 9) // UID + '.' + "txt"
        {
            continue;
        }
        strncpy(UID, dir->d_name, CLIENT_UID_SIZE - 1);
        UID[CLIENT_UID_SIZE - 1] = '\0';
        if (validUID(UID))
        {
            if (cur + CLIENT_UID_SIZE >= lenUsers - 2) // -2 to always to leave space for nl and null terminator
            {
                char *new = (char *)realloc(users, 2 * lenUsers);
                if (new == NULL)
                {
                    free(users);
                    return NULL;
                }
                memset(new + lenUsers, 0, lenUsers);
                users = new;
                lenUsers *= 2;
            }
            cur += sprintf(users + cur, "%s ", UID);
        }
    }

    // End users string - this will never SIGSEGV because of line 421
    users[cur] = '\n';
    users[cur + 1] = '\0';
    return users;
}

int userSubscribedToGroup(const char *UID, const char *GID)
{
    char dsGroupPath[DS_GROUPDIRPATH_SIZE];
    sprintf(dsGroupPath, "server/GROUPS/%s", GID);
    DIR *d;
    d = opendir(dsGroupPath);
    if (d == NULL)
    {
        return 0;
    }
    struct dirent *dir;
    char txtUID[CLIENT_UID_SIZE];
    while ((dir = readdir(d)) != NULL)
    {
        if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
        {
            continue;
        }
        if (strlen(dir->d_name) != 9) // UID + '.' + "txt"
        {
            continue;
        }
        strncpy(txtUID, dir->d_name, CLIENT_UID_SIZE - 1); // copy only 5 characters (len(UID))
        txtUID[CLIENT_UID_SIZE - 1] = '\0';
        if (!strcmp(txtUID, UID))
        {
            return 1;
        }
    }
    closedir(d);
    return 0;
}

int createMessageInGroup(char *newMID, const char *UID, const char *GID, int TSize, const char *Text)
{
    char dsGroupMsgPath[DS_GROUPMSGPATH_SIZE];
    sprintf(dsGroupMsgPath, "server/GROUPS/%s/MSG", GID);
    struct dirent **dir;

    // alphasort will sort the messages by ascending order
    int n = scandir(dsGroupMsgPath, &dir, 0, alphasort);
    int max = 0, flag = 0;
    if (n < 0)
    {
        return 0;
    }
    while (n--)
    {
        if (!flag)
        {
            if (validMID(dir[n]->d_name))
            {
                max = atoi(dir[n]->d_name); // + 1 because of new message
                flag = 1;
            }
        }
        free(dir[n]);
    }
    free(dir);

    // Create new message ID
    if (0 <= max && max <= 8)
    {
        sprintf(newMID, "000%d", max + 1);
    }
    else if (9 <= max && max <= 98)
    {
        sprintf(newMID, "00%d", max + 1);
    }
    else if (99 <= max && max <= 998)
    {
        sprintf(newMID, "0%d", max + 1);
    }
    else if (999 <= max && max <= 9998)
    {
        sprintf(newMID, "%d", max + 1);
    }
    else
    { // Message limit
        return 0;
    }

    // Create new message directory
    char newGroupMsgDSPath[DS_GROUPMSGDIRPATH_SIZE];
    sprintf(newGroupMsgDSPath, "server/GROUPS/%s/MSG/%s", GID, newMID);
    int ret = mkdir(newGroupMsgDSPath, 0700);
    if (ret == -1)
    {
        return 0;
    }

    // Create new message author file
    FILE *author;
    char newGroupMsgAuthorPath[DS_GROUPMSGAUTHORPATH_SIZE];
    sprintf(newGroupMsgAuthorPath, "server/GROUPS/%s/MSG/%s/A U T H O R.txt", GID, newMID);
    author = fopen(newGroupMsgAuthorPath, "w");
    if (author == NULL)
    {
        perror("[-] Post failed to create new msg author file");
        return 0;
    }
    if (fwrite(UID, sizeof(char), CLIENT_UID_SIZE - 1, author) != CLIENT_UID_SIZE - 1)
    {
        perror("[-] Post failed to write on new message author file");
        return 0;
    }
    if (fwrite("\n", sizeof(char), 1, author) != 1)
    { // Tejo ends author file with nl
        perror("[-] Post failed to write on new message author file (nl)");
        return 0;
    }
    if (fclose(author) == -1)
    {
        perror("[-] Post failed to close new msg author file");
        return 0;
    }

    // Create new message text file
    FILE *text;
    char newGroupMsgTextPath[DS_GROUPMSGTEXTPATH_SIZE];
    sprintf(newGroupMsgTextPath, "server/GROUPS/%s/MSG/%s/T E X T.txt", GID, newMID);
    text = fopen(newGroupMsgTextPath, "w");
    if (text == NULL)
    {
        return 0;
    }
    if (fwrite(Text, sizeof(char), TSize, text) != TSize)
    {
        return 0;
    }
    if (fclose(text) == -1)
    {
        return 0;
    }

    return 1;
}

int checkNumberOfMsgsToRet(const char *GID, int MID)
{
    struct dirent **dir;
    int num = 0;
    char dsGroupMsgPath[DS_GROUPMSGPATH_SIZE];
    sprintf(dsGroupMsgPath, "server/GROUPS/%s/MSG", GID);
    int n = scandir(dsGroupMsgPath, &dir, 0, alphasort);
    if (n < 0)
    {
        return -1;
    }
    for (int i = 0; i < n; ++i)
    {
        if (dir[i]->d_type == DT_DIR && validMID(dir[i]->d_name) && atoi(dir[i]->d_name) >= MID)
        {
            num++;
        }
        free(dir[i]);
    }
    free(dir);
    return (num > 20) ? 20 : num;
}

int retrieveDSGroupMessages(int fd, const char *GID, int startMID, int numMsgsToRet)
{
    struct dirent **msg;
    char dsGroupMsgPath[DS_GROUPMSGPATH_SIZE];
    sprintf(dsGroupMsgPath, "server/GROUPS/%s/MSG", GID);
    int n = scandir(dsGroupMsgPath, &msg, 0, alphasort);
    if (n < 0)
    { // scandir failed
        return 0;
    }
    int numMsgsRtvd = 0;
    for (int i = 0; i < n; ++i)
    {
        if (msg[i]->d_type == DT_DIR && validMID(msg[i]->d_name) && atoi(msg[i]->d_name) >= startMID && numMsgsRtvd < numMsgsToRet)
        { // We found a message directory from a message that is supposed to be retrieved
            char MID[DS_MID_SIZE] = "";
            strncpy(MID, msg[i]->d_name, DS_MID_SIZE - 1);
            MID[DS_MID_SIZE - 1] = '\0';
            // Open the message directory and check its content looking for a file

            char messageDSGroupPath[DS_GROUPMSGDIRPATH_SIZE];
            sprintf(messageDSGroupPath, "server/GROUPS/%s/MSG/%s", GID, MID);
            DIR *msgDir = opendir(messageDSGroupPath);
            if (msgDir == NULL)
            { // opendir failed
                return 0;
            }
            // Message file related variables
            char groupMsgFilePath[DS_GROUPMSGFILEPATH_SIZE] = "";
            char groupMsgFileName[PROTOCOL_FNAME_SIZE] = "";
            struct dirent *msgEntry;
            int msgFileFlag = NO_FILE;
            FILE *file;
            long fileLength;
            while ((msgEntry = readdir(msgDir)) != NULL)
            { // Check if message has a file attached
                if (!strcmp(".", msgEntry->d_name) || !strcmp("..", msgEntry->d_name) || !strcmp(msgEntry->d_name, "A U T H O R.txt") || !strcmp(msgEntry->d_name, "T E X T.txt"))
                {
                    continue;
                }
                if (msgEntry->d_type == DT_REG)
                { // Message has a file attached
                    msgFileFlag = HAS_FILE;
                    memset(groupMsgFilePath, 0, sizeof(groupMsgFilePath));
                    memset(groupMsgFileName, 0, sizeof(groupMsgFileName));
                    if (strlen(msgEntry->d_name) > 24)
                    { // Unexpected file name format -> NOK
                        return 0;
                    }
                    size_t lenFName = strlen(msgEntry->d_name);
                    strncpy(groupMsgFileName, msgEntry->d_name, lenFName);
                    groupMsgFileName[lenFName] = '\0';
                    sprintf(groupMsgFilePath, "server/GROUPS/%s/MSG/%s/%s", GID, MID, groupMsgFileName);
                    file = fopen(groupMsgFilePath, "rb");
                    if (file == NULL)
                    {
                        return 0;
                    }
                    if (fseek(file, 0, SEEK_END) == -1)
                    {
                        return 0;
                    }
                    fileLength = ftell(file);
                    if (fileLength == -1)
                    {
                        return 0;
                    }
                    rewind(file);
                    if (fclose(file) == -1)
                    {
                        return 0;
                    }
                }
            }
            if (closedir(msgDir) == -1)
            { // closedir failed
                return 0;
            }

            // Open author and text files and read them
            FILE *author;
            char groupMsgAuthorPath[DS_GROUPMSGAUTHORPATH_SIZE];
            int lenAuthor;
            sprintf(groupMsgAuthorPath, "server/GROUPS/%s/MSG/%s/A U T H O R.txt", GID, MID);
            FILE *text;
            char groupMsgTextPath[DS_GROUPMSGTEXTPATH_SIZE];
            int lenText;
            sprintf(groupMsgTextPath, "server/GROUPS/%s/MSG/%s/T E X T.txt", GID, MID);
            // A U T H O R.txt
            author = fopen(groupMsgAuthorPath, "r");
            if (author == NULL)
            {
                return 0;
            }
            if (fseek(author, 0, SEEK_END) == -1)
            {
                return 0;
            }
            lenAuthor = ftell(author) - 1; // author has \n so we avoid this extra byte
            if (lenAuthor == -2)
            {
                return 0;
            }
            rewind(author);
            if (lenAuthor != CLIENT_UID_SIZE - 1)
            { // Invalid author was written -> it must a valid client ID
                return 0;
            }
            // T E X T.txt
            text = fopen(groupMsgTextPath, "r");
            if (text == NULL)
            {
                return 0;
            }
            if (fseek(text, 0, SEEK_END) == -1)
            {
                return 0;
            }
            lenText = ftell(text);
            if (lenText == -1)
            {
                return 0;
            }
            rewind(text);
            if (lenText > 240)
            { // Size verification
                return 0;
            }
            // Allocate needed memory for both buffers
            char *msgAuthor = (char *)calloc(sizeof(char), lenAuthor + 1);
            if (!msgAuthor)
            {
                return 0;
            }
            char *msgText = (char *)calloc(sizeof(char), lenText + 1);
            if (!msgText)
            {
                free(msgAuthor);
                return 0;
            }
            // Fill both buffers
            if (fread(msgAuthor, sizeof(char), lenAuthor, author) == -1)
            {
                return 0;
            }
            if (fread(msgText, sizeof(char), lenText, text) == -1)
            {
                return 0;
            }
            if (fclose(author) == -1)
            {
                return 0;
            }
            if (fclose(text) == -1)
            {
                return 0;
            }
            msgAuthor[lenAuthor] = '\0';
            msgText[lenText] = '\0';
            if (!validUID(msgAuthor))
            { // Author verification
                return 0;
            }

            // Send a message to the client
            char msgTextMessage[DS_MSGTEXTINFO_SIZE] = "";
            sprintf(msgTextMessage, " %s %s %d %s", MID, msgAuthor, lenText, msgText);
            if (sendTCP(fd, msgTextMessage) == -1)
            {
                return 0;
            }

            // Sends a file if it has one to send
            if (msgFileFlag == HAS_FILE)
            {
                char msgFileMessage[DS_MSGFILEINFO_SIZE] = "";
                sprintf(msgFileMessage, " / %s %ld ", groupMsgFileName, fileLength);
                if (sendTCP(fd, msgFileMessage) == -1)
                {
                    return 0;
                }
                if (!sendFile(fd, groupMsgFilePath, fileLength))
                {
                    return 0;
                }
            }
            free(msgAuthor);
            free(msgText);
            numMsgsRtvd++;
        }
        free(msg[i]);
    }
    free(msg);
    // Every reply must end with a nl
    if (sendTCP(fd, "\n") == -1)
    {
        return 0;
    }

    // Wait for message confirmation from client
    // Since the message confirmation nature is ambiguous per the statement we assume a client won't send a
    // confirmation with more than 256 characters
    char clientRetrieveConfirmation[DS_RETCONFBUF_SIZE] = "";
    int b;
    if ((b = readTCP(fd, clientRetrieveConfirmation, DS_RETCONFBUF_SIZE)) == -1)
    {
        return 0;
    }
    clientRetrieveConfirmation[b] = '\0';
    return 1;
}