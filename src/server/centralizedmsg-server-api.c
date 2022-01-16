#include "centralizedmsg-server-api.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

char *processClientUDP(char *message)
{
    char *token, *tokenList[CLIENT_NUMTOKENS];
    int numTokens = 0;
    int cmd;
    char *response;
    token = strtok(message, " ");
    while (token)
    {
        tokenList[numTokens++] = token;
        token = strtok(NULL, " ");
    }
    cmd = parseDSClientCommand(tokenList[0]);
    switch (cmd)
    {
    case REGISTER:
        response = clientRegister(tokenList, numTokens);
        break;
    case UNREGISTER:
        response = clientUnregister(tokenList, numTokens);
        break;
    case LOGIN:
        response = clientLogin(tokenList, numTokens);
        break;
    case LOGOUT:
        response = clientLogout(tokenList, numTokens);
        break;
    case GROUPS:
        response = listDSGroups(numTokens);
        break;
    case SUBSCRIBE:
        response = clientSubscribeGroup(tokenList, numTokens);
        break;
    case UNSUBSCRIBE:
        response = clientUnsubscribeGroup(tokenList, numTokens);
        break;
    case MY_GROUPS:
        response = listClientDSGroups(tokenList, numTokens);
        break;
    default:
        response = strdup(ERR_MSG);
        break;
    }
    return response;
}

void processClientTCP(int fd, char *command)
{
    int cmd = parseDSClientCommand(command);
    switch (cmd)
    {
    case ULIST:
        showClientsInGroup(fd);
        break;
    case POST:
        clientPostInGroup(fd);
        break;
    case RETRIEVE:
        retrieveMessagesFromGroup(fd);
        break;
    default:
        if (sendTCP(fd, ERR_MSG) == -1)
        {
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Formats a message to be sent to the client from the DS via UDP protocol.
 *
 * @param command macro that contains the respective operation.
 * @param status string that contains the operation status.
 * @return message to be sent to the client via UDP protocol.
 */
static char *createDSUDPReply(int command, char *status)
{
    char message[DS_TO_CLIENT_UDP_SIZE];
    switch (command)
    {
    case REGISTER:
        sprintf(message, "RRG %s\n", status);
        break;
    case UNREGISTER:
        sprintf(message, "RUN %s\n", status);
        break;
    case LOGIN:
        sprintf(message, "RLO %s\n", status);
        break;
    case LOGOUT:
        sprintf(message, "ROU %s\n", status);
        break;
    case GROUPS:
        sprintf(message, "RGL %s\n", status);
        break;
    case SUBSCRIBE:
        sprintf(message, "RGS %s\n", status);
        break;
    case UNSUBSCRIBE:
        sprintf(message, "RGU %s\n", status);
        break;
    case MY_GROUPS:
        sprintf(message, "RGM %s\n", status);
        break;
    }
    return strdup(message);
}

/**
 * @brief Formats a message to be sent to the client from the DS via TCP protocol.
 *
 * @param fd file descriptor where the TCP connection was estabelished.
 * @param command macro that contains the respective operation.
 * @param status string that contains the operation status.
 */
static void sendDSStatusTCP(int fd, int command, char *status)
{
    char message[DS_TCPSTATUSBUF_SIZE];
    switch (command)
    {
    case ULIST:
        if (!strcmp(status, "NOK"))
        {
            sprintf(message, "RUL NOK\n");
        }
        else
        {
            sprintf(message, "RUL %s", status); // no nl here is intended
        }
        break;
    case POST:
        sprintf(message, "RPT %s\n", status);
        break;
    case RETRIEVE:
        if (!strcmp(status, "NOK") || !strcmp(status, "EOF"))
        {
            sprintf(message, "RRT %s\n", status);
        }
        else
        {
            sprintf(message, "RRT %s", status);
        }
    }
    if (sendTCP(fd, message) == -1)
    {
        close(fd);
        exit(EXIT_FAILURE);
    }
}

char *clientRegister(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // Wrong protocol message received
        return createDSUDPReply(REGISTER, "NOK");
    }
    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Wrong protocol message received
        return createDSUDPReply(REGISTER, "NOK");
    }

    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    char clientPwdPath[DS_CLIENTPWDPATH_SIZE];

    // Create user directory
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);
    int ret = mkdir(clientDirPath, 0700);
    if (ret == -1)
    {
        if (errno == EEXIST)
        { // This folder already exists -> duplicate user
            return createDSUDPReply(REGISTER, "DUP");
        }
        else
        { // Other errno error -> registration process failed (NOK)
            return createDSUDPReply(REGISTER, "NOK");
        }
    }

    // Create user password file
    sprintf(clientPwdPath, "server/USERS/%s/%s_pass.txt", tokenList[1], tokenList[1]);
    FILE *pwd = fopen(clientPwdPath, "w");
    if (pwd == NULL)
    {
        return createDSUDPReply(REGISTER, "NOK");
    }
    if (fwrite(tokenList[2], sizeof(char), CLIENT_PWD_SIZE - 1, pwd) != CLIENT_PWD_SIZE - 1)
    {
        return createDSUDPReply(REGISTER, "NOK");
    }
    if (fclose(pwd) == -1)
    {
        return createDSUDPReply(REGISTER, "NOK");
    }
    return createDSUDPReply(REGISTER, "OK");
}

char *clientUnregister(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // Wrong protocol message received
        return createDSUDPReply(UNREGISTER, "NOK");
    }
    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Wrong protocol message received
        return createDSUDPReply(UNREGISTER, "NOK");
    }

    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);

    if (!directoryExists(clientDirPath))
    { // User wasn't previously registered - no dir folder
        return createDSUDPReply(UNREGISTER, "NOK");
    }

    if (!passwordsMatch(tokenList[1], tokenList[2]))
    { // Given and stored passwords do not match
        return createDSUDPReply(UNREGISTER, "NOK");
    }

    if (!unsubscribeClientFromGroups(tokenList[1]))
    { // Failed to unsubscribe user from all of its subscribed groups
        return createDSUDPReply(UNREGISTER, "NOK");
    }

    if (!removeDirectory(clientDirPath))
    { // User directory failed to be removed
        return createDSUDPReply(UNREGISTER, "NOK");
    }

    return createDSUDPReply(UNREGISTER, "OK");
}

char *clientLogin(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // Wrong protocol message received
        return createDSUDPReply(LOGIN, "NOK");
    }

    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Wrong protocol message received
        return createDSUDPReply(LOGIN, "NOK");
    }

    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);

    if (!directoryExists(clientDirPath))
    { // User isn't registered -> can't login
        return createDSUDPReply(LOGIN, "NOK");
    }

    if (!passwordsMatch(tokenList[1], tokenList[2]))
    { // Given and stored passwords do not match
        return createDSUDPReply(LOGIN, "NOK");
    }

    // Create user login file
    char clientLoginPath[DS_CLIENTLOGINPATH_SIZE];
    sprintf(clientLoginPath, "server/USERS/%s/%s_login.txt", tokenList[1], tokenList[1]);
    FILE *login = fopen(clientLoginPath, "a"); // create dummy file
    if (login == NULL)
    {
        return createDSUDPReply(LOGIN, "NOK");
    }
    if (fclose(login) == -1)
    {
        return createDSUDPReply(LOGIN, "NOK");
    }

    return createDSUDPReply(LOGIN, "OK");
}

char *clientLogout(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // Wrong protocol message received
        return createDSUDPReply(LOGOUT, "NOK");
    }

    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Wrong protocol message received
        return createDSUDPReply(LOGOUT, "NOK");
    }

    // Check if user is registered
    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);
    if (!directoryExists(clientDirPath))
    {
        return createDSUDPReply(LOGOUT, "NOK");
    }

    // Check if given and stored passwords match
    if (!passwordsMatch(tokenList[1], tokenList[2]))
    {
        return createDSUDPReply(LOGOUT, "NOK");
    }

    // Delete the login file
    char clientLoginPath[DS_CLIENTLOGINPATH_SIZE];
    sprintf(clientLoginPath, "server/USERS/%s/%s_login.txt", tokenList[1], tokenList[1]);
    if (!access(clientLoginPath, F_OK))
    { // Login file exists
        if (!unlink(clientLoginPath))
        { // Login file was deleted
            return createDSUDPReply(LOGOUT, "OK");
        }
    }

    // If it gets here either unlink failed or user isn't logged in
    return createDSUDPReply(LOGOUT, "NOK");
}

char *listDSGroups(int numTokens)
{
    if (numTokens != 1)
    { // Wrong protocol message received (no NOK status in RGL)
        return strdup(ERR_MSG);
    }

    // Create groups list message
    char groupsDSBuf[DS_GROUPSLISTBUF_SIZE] = "";
    if (!createGroupListMessage(groupsDSBuf, NULL, 0))
    { // Failed to create group list messages (no NOK status in RGL)
        return strdup(ERR_MSG);
    }
    return createDSUDPReply(GROUPS, groupsDSBuf);
}

char *clientSubscribeGroup(char **tokenList, int numTokens)
{
    if (numTokens != 4)
    { // Wrong protocol message received
        return createDSUDPReply(SUBSCRIBE, "NOK");
    }
    if (!(validUID(tokenList[1]) && isGID(tokenList[2]) && validGName(tokenList[3])))
    { // Wrong protocol message received
        return createDSUDPReply(SUBSCRIBE, "NOK");
    }

    // Check if user is registered
    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);
    if (!directoryExists(clientDirPath))
    {
        return createDSUDPReply(SUBSCRIBE, "NOK");
    }

    // Check if user is logged in
    char clientLoginPath[DS_CLIENTLOGINPATH_SIZE];
    sprintf(clientLoginPath, "server/USERS/%s/%s_login.txt", tokenList[1], tokenList[1]);
    if (access(clientLoginPath, F_OK) != 0)
    {
        return createDSUDPReply(SUBSCRIBE, "E_USR");
    }

    // Check if given group ID exists
    int dsGroupNum = atoi(tokenList[2]);
    if (dsGroupNum > dsGroups.no_groups)
    {
        return createDSUDPReply(SUBSCRIBE, "E_GRP");
    }

    if (!strcmp(tokenList[2], "00"))
    { // Create a new group
        if (dsGroups.no_groups == 99)
        { // DS is full
            return createDSUDPReply(SUBSCRIBE, "E_FULL");
        }
        char newDSGID[DS_GID_SIZE];
        if (dsGroups.no_groups < 9)
        {
            sprintf(newDSGID, "0%d", dsGroups.no_groups + 1);
        }
        else
        {
            sprintf(newDSGID, "%d", dsGroups.no_groups + 1);
        }

        // Check if there's a group with that same name
        for (int i = 0; i < dsGroups.no_groups; ++i)
        {
            if (!strcmp(tokenList[3], dsGroups.groupinfo[i].name))
            {
                return createDSUDPReply(SUBSCRIBE, "E_GNAME");
            }
        }

        // Add new group to DS GROUPS directory
        char dsGroupPath[DS_GROUPDIRPATH_SIZE];
        char dsGroupMsgPath[DS_GROUPMSGPATH_SIZE];
        char dsGroupNamePath[DS_GNAMEPATH_SIZE];
        char dsGroupClientSubPath[DS_GROUPCLIENTSUBPATH_SIZE];
        char dsGroupName[DS_GNAME_SIZE + 1]; // +1 for \n
        int ret;

        // Create group directory
        sprintf(dsGroupPath, "server/GROUPS/%s", newDSGID);
        ret = mkdir(dsGroupPath, 0700);
        if (ret == -1)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }

        // Create group messages directory
        sprintf(dsGroupMsgPath, "server/GROUPS/%s/MSG", newDSGID);
        ret = mkdir(dsGroupMsgPath, 0700);
        if (ret == -1)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }

        // Create group name file
        sprintf(dsGroupNamePath, "server/GROUPS/%s/%s_name.txt", newDSGID, newDSGID);
        FILE *name = fopen(dsGroupNamePath, "w");
        if (name == NULL)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }
        sprintf(dsGroupName, "%s\n", tokenList[3]);
        size_t lenDSGroupName = strlen(dsGroupName);
        if (fwrite(dsGroupName, sizeof(char), lenDSGroupName, name) != lenDSGroupName)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }
        if (fclose(name) == -1)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }

        // Create group client subscribe file
        sprintf(dsGroupClientSubPath, "server/GROUPS/%s/%s.txt", newDSGID, tokenList[1]);
        FILE *client = fopen(dsGroupClientSubPath, "a");
        if (client == NULL)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }
        if (fclose(client) == -1)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }

        // Add new group to global struct
        int n = dsGroups.no_groups;
        strcpy(dsGroups.groupinfo[n].name, tokenList[3]);
        strcpy(dsGroups.groupinfo[n].no, newDSGID);
        (&dsGroups)->no_groups++;

        // Create new group status message
        char newGroupStatus[DS_NEWGROUPSTATUS_SIZE];
        sprintf(newGroupStatus, "NEW %s", newDSGID);
        return createDSUDPReply(SUBSCRIBE, newGroupStatus);
    }
    // This else is safe to assume because isGID has already made sure that the given GID is valid (00 - 99)
    else
    { // Subscribe to existing group

        if (!groupNamesMatch(tokenList[2], tokenList[3]))
        { // Check if given group name matches the stored one
            fprintf(stderr, "[-] Wrong group name given.\n");
            return createDSUDPReply(SUBSCRIBE, "E_GNAME");
        }

        // Create subscribed user file
        char dsGroupClientSubPath[DS_GROUPCLIENTSUBPATH_SIZE];
        sprintf(dsGroupClientSubPath, "server/GROUPS/%s/%s.txt", tokenList[2], tokenList[1]);
        FILE *client = fopen(dsGroupClientSubPath, "a");
        if (client == NULL)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }
        if (fclose(client) == -1)
        {
            return createDSUDPReply(SUBSCRIBE, "NOK");
        }
        return createDSUDPReply(SUBSCRIBE, "OK");
    }
}

char *clientUnsubscribeGroup(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // Wrong protocol message received
        return createDSUDPReply(UNSUBSCRIBE, "NOK");
    }

    if (!(validUID(tokenList[1]) && validGID(tokenList[2])))
    { // Wrong protocol message received
        return createDSUDPReply(UNSUBSCRIBE, "NOK");
    }

    // Check if user is registered
    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);
    if (!directoryExists(clientDirPath))
    {
        return createDSUDPReply(UNSUBSCRIBE, "E_USR");
    }

    // Check if user is logged in
    char clientLoginPath[DS_CLIENTLOGINPATH_SIZE];
    sprintf(clientLoginPath, "server/USERS/%s/%s_login.txt", tokenList[1], tokenList[1]);
    if (access(clientLoginPath, F_OK) != 0)
    {
        return createDSUDPReply(UNSUBSCRIBE, "E_USR");
    }

    // Check if given group ID exists
    int dsGroupNum = atoi(tokenList[2]);
    if (dsGroupNum > dsGroups.no_groups)
    {
        return createDSUDPReply(UNSUBSCRIBE, "E_GRP");
    }

    // Delete subscribed user file
    char dsGroupClientSubPath[DS_GROUPCLIENTSUBPATH_SIZE];
    sprintf(dsGroupClientSubPath, "server/GROUPS/%s/%s.txt", tokenList[2], tokenList[1]);
    if (unlink(dsGroupClientSubPath) != 0 && errno != ENOENT)
    { // errno = ENOENT means that the file doesn't exist -> the UID wasn't subscribed
        return createDSUDPReply(UNSUBSCRIBE, "NOK");
    }

    return createDSUDPReply(UNSUBSCRIBE, "OK");
}

char *listClientDSGroups(char **tokenList, int numTokens)
{
    if (numTokens != 2)
    { // Wrong protocol message received (no NOK status in RGM)
        return strdup(ERR_MSG);
    }

    if (!(validUID(tokenList[1])))
    { // Wrong protocol message received (no NOK status in RGM)
        return strdup(ERR_MSG);
    }

    // Check if user is registered
    char clientDirPath[DS_CLIENTDIRPATH_SIZE];
    sprintf(clientDirPath, "server/USERS/%s", tokenList[1]);
    if (!directoryExists(clientDirPath))
    {
        return createDSUDPReply(MY_GROUPS, "E_USR");
    }

    // Check if user is logged in
    char clientLoginPath[DS_CLIENTLOGINPATH_SIZE];
    sprintf(clientLoginPath, "server/USERS/%s/%s_login.txt", tokenList[1], tokenList[1]);
    if (access(clientLoginPath, F_OK) != 0)
    {
        return createDSUDPReply(MY_GROUPS, "E_USR");
    }

    // Fill variables that contain information about the groups that the client is subscribed to
    int clientGroupsSubscribed[DS_MAX_NUM_GROUPS];
    int numGroupsSub;
    if (!fillClientSubscribedGroups(tokenList[1], clientGroupsSubscribed, &numGroupsSub))
    {
        return strdup(ERR_MSG);
    }

    // Create client groups list message
    char clientGroupsDSBuf[DS_GROUPSLISTBUF_SIZE] = "";
    if (!createGroupListMessage(clientGroupsDSBuf, clientGroupsSubscribed, numGroupsSub))
    { // Failed to create client group list messages (no NOK status in RGM)
        return strdup(ERR_MSG);
    }

    return createDSUDPReply(MY_GROUPS, clientGroupsDSBuf);
}

void showClientsInGroup(int fd)
{
    // Read the group ID to ULS command
    char GID[DS_GID_SIZE];
    int n;
    if ((n = readTCP(fd, GID, DS_GID_SIZE)) == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Check if message is valid according to protocol
    if (GID[n - 1] != '\n')
    { // Wrong protocol message was received
        exit(EXIT_FAILURE);
    }
    GID[n - 1] = '\0';

    // Check if valid GID
    if (!validGID(GID))
    {
        sendDSStatusTCP(fd, ULIST, "NOK");
        return;
    }

    // Check if group exists
    char dsGroupPath[DS_GROUPDIRPATH_SIZE];
    sprintf(dsGroupPath, "server/GROUPS/%s", GID);
    if (!directoryExists(dsGroupPath))
    {
        sendDSStatusTCP(fd, ULIST, "NOK");
        return;
    }

    // Send to client initial message
    char ulistInitialStatus[DS_TCPSTATUSBUF_SIZE - PROTOCOL_CODE_SIZE];
    sprintf(ulistInitialStatus, "OK %s ", GID);
    sendDSStatusTCP(fd, ULIST, ulistInitialStatus);

    // Iterate over group's directory and print out users
    char *response = listUsersInDSGroup(GID);
    if (response == NULL)
    { // Something went wrong while listing users in group
        sendDSStatusTCP(fd, ULIST, "NOK");
    }
    if (sendTCP(fd, response) == -1)
    {
        free(response);
        close(fd);
        exit(EXIT_FAILURE);
    }
    free(response);
}

void clientPostInGroup(int fd)
{
    int n;
    int i;
    char singleCharDS[CHAR_SIZE];

    // Read UID and check if it's a valid protocol message and a valid UID
    char UID[CLIENT_UID_SIZE];
    if ((n = readTCP(fd, UID, CLIENT_UID_SIZE)) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (UID[n - 1] != ' ')
    { // There must be a space between the UID and GID
        exit(EXIT_FAILURE);
    }
    UID[n - 1] = '\0';
    if (!validUID(UID))
    { // Tejo aborts upon invalid UID
        exit(EXIT_FAILURE);
    }

    // Read GID and check if it's a valid protocol message and a valid GID
    char GID[DS_GID_SIZE];
    if ((n = readTCP(fd, GID, DS_GID_SIZE)) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (GID[n - 1] != ' ')
    { // There must be a space between the GID and TSize
        exit(EXIT_FAILURE);
    }
    GID[n - 1] = '\0';
    if (!validGID(GID))
    { // Tejo aborts upon invalid GID
        exit(EXIT_FAILURE);
    }

    // Read message TSize and check if it's a valid text size
    char TSizeBuf[PROTOCOL_TEXTSZ_SIZE] = "";
    for (i = 0; i < PROTOCOL_TEXTSZ_SIZE; ++i)
    {
        if ((n = readTCP(fd, singleCharDS, CHAR_SIZE - 1)) == -1)
        {
            exit(EXIT_FAILURE);
        }
        if (singleCharDS[0] != ' ')
        {
            strcat(TSizeBuf, &singleCharDS[0]);
        }
        else
        {
            break;
        }
    }
    TSizeBuf[i] = '\0';
    if (singleCharDS[0] != ' ')
    { // Wrong protocol message
        exit(EXIT_FAILURE);
    }
    if (!isNumber(TSizeBuf) || atoi(TSizeBuf) > 240)
    { // Tejo aborts upon invalid TSize or greater than 240
        exit(EXIT_FAILURE);
    }

    // Read message text and check if it's valid
    int TSize = atoi(TSizeBuf);
    char Text[PROTOCOL_TEXT_SIZE];
    if ((n = readTCP(fd, Text, TSize)) == -1)
    {
        exit(EXIT_FAILURE);
    }

    // Create new message in group
    char newMID[DS_MID_SIZE] = "";
    if (!createMessageInGroup(newMID, UID, GID, TSize, Text))
    {
        sendDSStatusTCP(fd, POST, "NOK");
    }

    // Check if it has a file
    if ((n = readTCP(fd, singleCharDS, CHAR_SIZE - 1)) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (singleCharDS[0] == '\n')
    { // Only text was sent
        if (!userSubscribedToGroup(UID, GID))
        { // In order to prevent connection reset by peer and not getting the full message from the client
          // we only check if the client is subscribed to the given group after receiving the whole message from it
            sendDSStatusTCP(fd, POST, "NOK");
            char newGroupMsgDSPath[DS_GROUPMSGDIRPATH_SIZE];
            sprintf(newGroupMsgDSPath, "server/GROUPS/%s/MSG/%s", GID, newMID);
            if (!removeDirectory(newGroupMsgDSPath))
            {
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            sendDSStatusTCP(fd, POST, newMID);
        }
        return;
    }
    else if (singleCharDS[0] == ' ')
    { // There's a file attached to it too
        char FName[PROTOCOL_FNAME_SIZE] = "";
        char FSizeBuf[PROTOCOL_FILESZ_SIZE] = "";

        // Read file name
        for (i = 0; i < PROTOCOL_FNAME_SIZE; ++i)
        {
            if ((n = readTCP(fd, singleCharDS, CHAR_SIZE - 1)) == -1)
            {
                exit(EXIT_FAILURE);
            }
            if (singleCharDS[0] != ' ')
            {
                strcat(FName, &singleCharDS[0]);
            }
            else
            {
                break;
            }
        }
        FName[i] = '\0';
        if (singleCharDS[0] != ' ')
        { // Tejo aborts on wrong protocol message
            exit(EXIT_FAILURE);
        }
        if (!validFName(FName))
        { // Tejo aborts on wrong protocol message
            exit(EXIT_FAILURE);
        }

        // Read file size
        for (i = 0; i < PROTOCOL_FILESZ_SIZE; ++i)
        {
            if ((n = readTCP(fd, singleCharDS, CHAR_SIZE - 1)) == -1)
            {
                exit(EXIT_FAILURE);
            }
            if (singleCharDS[0] != ' ')
            {
                strcat(FSizeBuf, &singleCharDS[0]);
            }
            else
            {
                break;
            }
        }
        FSizeBuf[i] = '\0';
        if (singleCharDS[0] != ' ')
        { // Tejo aborts on wrong protocol message
            exit(EXIT_FAILURE);
        }
        if (strlen(FSizeBuf) > 10)
        { // Tejo aborts on wrong protocol message
            exit(EXIT_FAILURE);
        }
        long FSize = atol(FSizeBuf);

        // Receive file
        char newGroupMsgFilePath[DS_GROUPMSGFILEPATH_SIZE];
        sprintf(newGroupMsgFilePath, "server/GROUPS/%s/MSG/%s/%s", GID, newMID, FName);
        if (!recvFile(fd, newGroupMsgFilePath, FSize))
        {
            sendDSStatusTCP(fd, POST, "NOK");
        }

        // All requests must end with a nl
        if ((n = readTCP(fd, singleCharDS, CHAR_SIZE - 1)) == -1)
        {
            exit(EXIT_FAILURE);
        }
        if (singleCharDS[0] != '\n')
        {
            sendTCP(fd, ERR_MSG);
            exit(EXIT_FAILURE); // No verification cause it'll exit with failure either way
        }

        if (!userSubscribedToGroup(UID, GID))
        { // In order to prevent connection reset by peer and not getting the full message from the client
          // we only check if the client is subscribed to the given group after receiving the whole message from it
            sendDSStatusTCP(fd, POST, "NOK");
            char newGroupMsgDSPath[DS_GROUPMSGDIRPATH_SIZE];
            sprintf(newGroupMsgDSPath, "server/GROUPS/%s/MSG/%s", GID, newMID);
            if (!removeDirectory(newGroupMsgDSPath))
            {
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            sendDSStatusTCP(fd, POST, newMID);
        }
    }
    else
    { // Wrong protocol message - Tejo aborts
        exit(EXIT_FAILURE);
    }
}

void retrieveMessagesFromGroup(int fd)
{
    int n;
    // Read UID and check if it's a valid protocol message and a valid UID
    char UID[CLIENT_UID_SIZE];
    if ((n = readTCP(fd, UID, CLIENT_UID_SIZE)) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (UID[n - 1] != ' ')
    { // There must be a space between the UID and GID
        exit(EXIT_FAILURE);
    }
    UID[n - 1] = '\0';
    if (!validUID(UID))
    { // Tejo aborts upon invalid UID
        exit(EXIT_FAILURE);
    }
    // Read GID and check if it's a valid protocol message and a valid GID
    char GID[DS_GID_SIZE];
    if ((n = readTCP(fd, GID, DS_GID_SIZE)) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (GID[n - 1] != ' ')
    { // There must be a space between the GID and TSize
        exit(EXIT_FAILURE);
    }
    GID[n - 1] = '\0';
    if (!validGID(GID))
    { // Tejo aborts upon invalid GID
        exit(EXIT_FAILURE);
    }
    // Check if user is subscribed to group with ID GID
    if (!userSubscribedToGroup(UID, GID))
    {
        sendDSStatusTCP(fd, RETRIEVE, "NOK");
        return;
    }

    // Read MID and check if it's a valid protocol message and a valid MID
    char MID[DS_MID_SIZE];
    if ((n = readTCP(fd, MID, DS_MID_SIZE)) == -1)
    {
        exit(EXIT_FAILURE);
    }
    if (MID[n - 1] != '\n')
    { // Every request must end with a nl
        sendTCP(fd, ERR_MSG);
        exit(EXIT_FAILURE);
    }
    // Check number of messages to retrieve and send initial message
    int startMID = atoi(MID);
    int numMsgsToRet = checkNumberOfMsgsToRet(GID, startMID);
    if (numMsgsToRet == -1)
    {
        sendDSStatusTCP(fd, RETRIEVE, "NOK");
        return;
    }
    else if (numMsgsToRet == 0)
    {
        sendDSStatusTCP(fd, RETRIEVE, "EOF");
        return;
    }
    // Send initial message
    char retrieveInitialStatus[DS_RETINITSTATUS_SIZE];
    sprintf(retrieveInitialStatus, "OK %d", numMsgsToRet);
    sendDSStatusTCP(fd, RETRIEVE, retrieveInitialStatus);
    // Retrieve all requested messages
    if (!retrieveDSGroupMessages(fd, GID, startMID, numMsgsToRet))
    {
        sendDSStatusTCP(fd, RETRIEVE, "NOK");
        return;
    }
}