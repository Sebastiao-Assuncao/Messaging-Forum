#include "centralizedmsg-client-api.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* DS Server information variables */
char addrDS[DS_ADDR_SIZE] = DS_DEFAULT_ADDR;
char portDS[DS_PORT_SIZE] = DS_DEFAULT_PORT;

/* UDP Socket related variables */
int fdDSUDP;
struct addrinfo hintsUDP, *resUDP;
struct sockaddr_in addrUDP;
socklen_t addrlenUDP;

/* TCP Socket related variables */
int fdDSTCP;
struct addrinfo hintsTCP, *resTCP;

/* Client current session variables */
int clientSession; // LOGGED_IN or LOGGED_OUT
char activeClientUID[CLIENT_UID_SIZE], activeClientPWD[CLIENT_PWD_SIZE];

/* Client DS group selected variable */
char activeDSGID[DS_GID_SIZE];

/* Message to DS via UDP protocol variable */
char messageToDSUDP[CLIENT_TO_DS_UDP_SIZE];

static void failDSUDP()
{
    closeUDPSocket(fdDSUDP, resUDP);
    exit(EXIT_FAILURE);
}

static void errDSUDP()
{
    fprintf(stderr, "[-] Wrong protocol message received from server via UDP. Program will now exit.\n");
    failDSUDP();
}

static void failDSTCP()
{
    closeUDPSocket(fdDSUDP, resUDP);
    closeTCPSocket(fdDSTCP, resTCP);
    exit(EXIT_FAILURE);
}

static void errDSTCP()
{
    fprintf(stderr, "[-] Wrong protocol message received from server via TCP. Program will now exit.\n");
    failDSTCP();
}

static void displayGroups(char *message, int numGroups)
{
    if (numGroups == 0)
    {
        printf("[+] There are no available groups.\n");
        return;
    }
    else if (numGroups == 1)
    {
        printf("[+] %d group: (GID | GName | Last MID)\n", numGroups);
    }
    else
    {
        printf("[+] %d groups: (GID | GName | Last MID)\n", numGroups);
    }
    // Safe to assume if it gets here since numGroups is a positive number
    // len(GLS 10) = 6; len(GLS 9) = 5
    char *p_message = (numGroups >= 10) ? message + 6 : message + 5;
    char GID[DS_GID_SIZE], GName[DS_GNAME_SIZE], MID[DS_MID_SIZE];
    int offsetPtr;
    while (numGroups--)
    {
        sscanf(p_message, " %s %s %s%n", GID, GName, MID, &offsetPtr);
        if (!(validGID(GID) && validGName(GName) && isMID(MID)))
        {
            errDSUDP();
        }
        printf("-> %2s | %24s | %4s\n", GID, GName, MID);
        p_message += offsetPtr;
    }
}

void createDSUDPSocket()
{
    fdDSUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (fdDSUDP == -1)
    {
        perror("[-] Client UDP socket failed to create");
        exit(EXIT_FAILURE);
    }
    memset(&hintsUDP, 0, sizeof(hintsUDP));
    hintsUDP.ai_family = AF_INET;
    hintsUDP.ai_socktype = SOCK_DGRAM;
    int errcode = getaddrinfo(addrDS, portDS, &hintsUDP, &resUDP);
    if (errcode != 0)
    {
        perror("[-] Failed on UDP address translation");
        close(fdDSUDP);
        exit(EXIT_FAILURE);
    }
}

void processDSUDPReply(char *message)
{
    char codeDS[PROTOCOL_CODE_SIZE], statusDS[PROTOCOL_STATUS_UDP_SIZE];
    sscanf(message, "%s %s", codeDS, statusDS);
    if (!strcmp(codeDS, "RRG"))
    { // REGISTER command
        if (!strcmp(statusDS, "OK"))
        {
            printf("[+] You have successfully registered.\n");
        }
        else if (!strcmp(statusDS, "DUP"))
        {
            fprintf(stderr, "[-] This user is already registered in the DS database.\n");
        }
        else if (!strcmp(statusDS, "NOK"))
        {
            fprintf(stderr, "[-] Something went wrong with register. Please try again later.\n");
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "RUN"))
    { // UNREGISTER command
        if (!strcmp(statusDS, "OK"))
        {
            printf("[+] You have successfully unregistered.\n");
        }
        else if (!strcmp(statusDS, "NOK"))
        {
            fprintf(stderr, "[-] Something went wrong with unregister. Please check user credentials and try again.\n");
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "RLO"))
    { // LOGIN command
        if (!strcmp(statusDS, "OK"))
        {
            clientSession = LOGGED_IN;
            printf("[+] You have successfully logged in.\n");
        }
        else if (!strcmp(statusDS, "NOK"))
        {
            fprintf(stderr, "[-] Something went wrong with login. Please check user credentials and try again.\n");
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "ROU"))
    { // LOGOUT command
        if (!strcmp(statusDS, "OK"))
        {
            clientSession = LOGGED_OUT;
            printf("[+] You have successfully logged out.\n");
        }
        else if (!strcmp(statusDS, "NOK"))
        {
            fprintf(stderr, "[-] Something went wrong with logout. Please try again and/or contact the developers.\n");
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "RGL"))
    { // GROUPS command
        if (isNumber(statusDS))
        {
            displayGroups(message, atoi(statusDS));
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "RGS"))
    { // SUBSCRIBE command
        if (!strcmp(statusDS, "OK"))
        {
            printf("[+] You have successfully subscribed to this group.\n");
        }
        else if (!strcmp(statusDS, "NEW"))
        {
            char newGID[DS_GID_SIZE];
            strncpy(newGID, message + 8, 2); // copy the new group ID : len(RGS NEW ) = 8
            newGID[DS_GID_SIZE - 1] = '\0';
            printf("[+] You have successfully created a new group (ID = %s).\n", newGID);
        }
        else if (!strcmp(statusDS, "E_FULL"))
        {
            fprintf(stderr, "[-] Group creation has failed. Maximum number of groups has been reached.\n");
        }
        else if (!strcmp(statusDS, "E_USR"))
        {
            fprintf(stderr, "[-] UID submitted to server is incorrect. Please try again and/or contact the developers.\n");
        }
        else if (!strcmp(statusDS, "E_GRP"))
        {
            fprintf(stderr, "[-] Group ID submitted is invalid. Please check available groups using 'gl' command and try again.\n");
        }
        else if (!strcmp(statusDS, "E_GNAME"))
        {
            fprintf(stderr, "[-] Group name submitted is invalid. Please try again.\n");
        }
        else if (!strcmp(statusDS, "NOK"))
        {
            fprintf(stderr, "[-] The group subscribing process has failed. Please try again.\n");
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "RGU"))
    { // UNSUBSCRIBE command
        if (!strcmp(statusDS, "OK"))
        {
            printf("[+] You have successfully unsubscribed to this group.\n");
        }
        else if (!strcmp(statusDS, "E_USR"))
        {
            fprintf(stderr, "[-] UID submitted to server is incorrect. Please try again and/or contact the developers.\n");
        }
        else if (!strcmp(statusDS, "E_GRP"))
        {
            fprintf(stderr, "[-] Invalid given group ID. Please check available groups using 'gl' command and try again.\n");
        }
        else
        {
            errDSUDP();
        }
    }
    else if (!strcmp(codeDS, "RGM"))
    { // MY_GROUPS command
        if (isNumber(statusDS))
        {
            displayGroups(message, atoi(statusDS));
        }
        else
        {
            errDSUDP();
        }
    }
    else
    { // Unexpected protocol DS reply was made
        errDSUDP();
    }
}

void exchangeDSUDPMsg(char *message)
{
    int numTries = DEFAULT_UDPRECV_TRIES;
    ssize_t n;

    while (numTries-- > 0)
    {
        // Client -> DS message
        n = sendto(fdDSUDP, message, strlen(message), 0, resUDP->ai_addr, resUDP->ai_addrlen); // strlen counts nl
        if (n == -1)
        { // Syscall failed -> terminate gracefully
            perror("[-] UDP message failed to send");
            failDSUDP();
        }
        // Start recvfrom timer
        if (timerOn(fdDSUDP) == -1)
        {
            perror("[-] Failed to set read timeout on UDP socket");
            failDSUDP();
        }
        // DS -> Client message
        addrlenUDP = sizeof(addrUDP);
        char dsReply[DS_TO_CLIENT_UDP_SIZE];
        n = recvfrom(fdDSUDP, dsReply, DS_TO_CLIENT_UDP_SIZE, 0, (struct sockaddr *)&addrUDP, &addrlenUDP);
        if (n == -1)
        {
            if ((errno == EAGAIN || errno == EWOULDBLOCK) && (numTries > 0))
            {
                continue;
            }
            else
            {
                perror("[-] Failed to receive UDP message");
                failDSUDP();
            }
        }
        // Turn timer off if message was received
        if (timerOff(fdDSUDP) == -1)
        {
            perror("[-] Failed to reset read timeout on UDP socket");
            failDSUDP();
        }
        dsReply[n] = '\0';
        if (dsReply[n - 1] != '\n')
        { // Each request/reply ends with newline according to DS-Client communication protocol
            fprintf(stderr, "[-] Wrong protocol message received from server via UDP. Program will now exit.\n");
            failDSUDP();
        }
        dsReply[n - 1] = '\0'; // Replace \n with \0
        processDSUDPReply(dsReply);
        break;
    }
}

void clientRegister(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // REG UID PWD
        fprintf(stderr, "[-] Incorrect register command usage. Please try again.\n");
        return;
    }
    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Protocol validation
        fprintf(stderr, "[-] Invalid register command arguments. Please check given UID and/or password and try again.\n");
        return;
    }
    sprintf(messageToDSUDP, "REG %s %s\n", tokenList[1], tokenList[2]);
    exchangeDSUDPMsg(messageToDSUDP);
}

void clientUnregister(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // UNREGISTER UID PWD / UNR UID PWD
        fprintf(stderr, "[-] Incorrect unregister command usage. Please try again.\n");
        return;
    }
    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Protocol validation
        fprintf(stderr, "[-] Invalid unregister command arguments. Please check given UID and/or password and try again.\n");
        return;
    }

    // If the client is logged in we have to locally log out
    if (clientSession == LOGGED_IN && !strcmp(tokenList[1], activeClientUID))
    {
        memset(activeClientUID, 0, sizeof(activeClientUID));
        memset(activeClientPWD, 0, sizeof(activeClientPWD));
        clientSession = LOGGED_OUT;
    }
    sprintf(messageToDSUDP, "UNR %s %s\n", tokenList[1], tokenList[2]);
    exchangeDSUDPMsg(messageToDSUDP);
}

void clientLogin(char **tokenList, int numTokens)
{
    if (clientSession == LOGGED_IN)
    { // Client is already logged in into an account
        fprintf(stderr, "[-] You're already logged in. Please logout before you try to log in again.\n");
        return;
    }
    if (numTokens != 3)
    { // LOGIN UID PWD
        fprintf(stderr, "[-] Incorrect login command usage. Please try again.\n");
        return;
    }
    if (!(validUID(tokenList[1]) && validPW(tokenList[2])))
    { // Protocol message
        fprintf(stderr, "[-] Invalid login command arguments. Please check given UID and/or password and try again.\n");
        return;
    }
    sprintf(messageToDSUDP, "LOG %s %s\n", tokenList[1], tokenList[2]);
    exchangeDSUDPMsg(messageToDSUDP);
    if (clientSession == LOGGED_IN)
    { // The exchangeDSUDPMsg function sets the client session to LOGGED_IN if reply is OK
        strcpy(activeClientUID, tokenList[1]);
        strcpy(activeClientPWD, tokenList[2]);
    }
}

void clientLogout(char **tokenList, int numTokens)
{
    if (clientSession == LOGGED_OUT)
    { // Client isn't logged in into any account
        fprintf(stderr, "[-] You're not logged in into any account.\n");
        return;
    }
    if (numTokens != 1)
    { // LOGOUT
        fprintf(stderr, "[-] Incorrect logout command usage. Please try again.\n");
        return;
    }
    sprintf(messageToDSUDP, "OUT %s %s\n", activeClientUID, activeClientPWD);
    exchangeDSUDPMsg(messageToDSUDP);
    if (clientSession == LOGGED_OUT)
    { // The exchangeDSUDPMsg function sets the client session to LOGGED_OUT if reply is OK
        memset(activeClientUID, 0, sizeof(activeClientUID));
        memset(activeClientPWD, 0, sizeof(activeClientPWD));
    }
}

void showCurrentClient(int numTokens)
{
    if (numTokens != 1)
    { // SHOWUID / SU
        fprintf(stderr, "[-] Incorrect showuid command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_IN)
    {
        printf("[+] You're logged in with user ID %s.\n", activeClientUID);
    }
    else
    {
        printf("[-] You're not logged in into any account.\n");
    }
}

void clientExit(int numTokens)
{
    if (numTokens != 1)
    { // EXIT
        fprintf(stderr, "[-] Incorrect exit command usage. Please try again.\n");
        return;
    }
    closeUDPSocket(fdDSUDP, resUDP);
    printf("[+] Exiting...\n");
    exit(EXIT_SUCCESS);
}

void showDSGroups(int numTokens)
{
    if (numTokens != 1)
    { // GROUPS / GL
        fprintf(stderr, "[-] Incorrect groups list command usage. Please try again.\n");
        return;
    }
    sprintf(messageToDSUDP, "GLS\n");
    exchangeDSUDPMsg(messageToDSUDP);
}

void clientSubscribeGroup(char **tokenList, int numTokens)
{
    if (numTokens != 3)
    { // S <CODE> GNAME
        fprintf(stderr, "[-] Incorrect subscribe command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_OUT)
    { // User must be logged in in order to subscribe to a group
        fprintf(stderr, "[-] Please login before you subscribe to a group.\n");
        return;
    }
    if (!strcmp(tokenList[1], "0"))
    { // Create a new group
        if (!validGName(tokenList[2]))
        {
            fprintf(stderr, "[-] Invalid new group name. Please try again.\n");
            return;
        }
        sprintf(messageToDSUDP, "GSR %s 00 %s\n", activeClientUID, tokenList[2]);
    }
    else if (validGID(tokenList[1]) && validGName(tokenList[2]))
    { // Subscribe to a group
        sprintf(messageToDSUDP, "GSR %s %s %s\n", activeClientUID, tokenList[1], tokenList[2]);
    }
    else
    {
        fprintf(stderr, "[-] Incorrect subscribe command usage. Please try again.\n");
        return;
    }
    exchangeDSUDPMsg(messageToDSUDP);
}

void clientUnsubscribeGroup(char **tokenList, int numTokens)
{
    if (numTokens != 2)
    { // UNSUBSCRIBE GID
        fprintf(stderr, "[-] Incorrect unsubscribe command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_OUT)
    { // User must be logged in order to unsubscribe to a group
        fprintf(stderr, "[-] Please login before you unsubscribe to a group.\n");
        return;
    }
    if (!validGID(tokenList[1]))
    {
        fprintf(stderr, "[-] Invalid given group ID to unsubscribe. Please try again.\n");
        return;
    }
    // Warn client that they're trying to unsubscribe group that is selected
    if (strlen(activeDSGID) > 0)
    { // There's a group selected
        if (!strcmp(activeDSGID, tokenList[1]))
        {
            printf("[!] The selected group is the one you're trying to unsubscribe.\n");
        }
    }
    sprintf(messageToDSUDP, "GUR %s %s\n", activeClientUID, tokenList[1]);
    exchangeDSUDPMsg(messageToDSUDP);
}

void clientShowSubscribedGroups(int numTokens)
{
    if (numTokens != 1)
    { // MY_GROUPS / MGL
        fprintf(stderr, "[-] Incorrect user groups' list command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_OUT)
    {
        fprintf(stderr, "[-] Please login before you request for your subscribed groups list.\n");
        return;
    }
    sprintf(messageToDSUDP, "GLM %s\n", activeClientUID);
    exchangeDSUDPMsg(messageToDSUDP);
}

void clientSelectGroup(char **tokenList, int numTokens)
{
    if (numTokens != 2)
    { // SELECT GID
        fprintf(stderr, "[-] Incorrect select command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_OUT)
    {
        fprintf(stderr, "[-] Please login before you select a group.\n");
        return;
    }
    if (!validGID(tokenList[1]))
    {
        fprintf(stderr, "[-] Invalid given group ID to select. Please try again.\n");
        return;
    }
    strcpy(activeDSGID, tokenList[1]);
    printf("[+] You have successfully selected group %s.\n[!] Make sure you've chosen a GID that actually exists in the DS's database. For further details use command 'gl'.\n", activeDSGID);
}

void showCurrentSelectedGID(int numTokens)
{
    if (numTokens != 1)
    { // SHOWGID / SG
        fprintf(stderr, "[-] Incorrect show select group command usage. Please try again.\n");
        return;
    }
    if (strlen(activeDSGID) > 0)
    {
        printf("[+] Group %s is selected.\n", activeDSGID);
    }
    else
    {
        printf("[-] You haven't selected any group yet.\n");
    }
}

void connectDSTCPSocket()
{
    fdDSTCP = socket(AF_INET, SOCK_STREAM, 0);
    if (fdDSTCP == -1)
    {
        perror("[-] Client TCP socket failed to create");
        failDSUDP();
    }
    memset(&hintsTCP, 0, sizeof(hintsTCP));
    hintsTCP.ai_family = AF_INET;
    hintsTCP.ai_socktype = SOCK_STREAM;
    int errcode = getaddrinfo(addrDS, portDS, &hintsTCP, &resTCP);
    if (errcode != 0)
    {
        perror("[-] Failed on TCP address translation");
        close(fdDSTCP);
        failDSUDP();
    }
    int n = connect(fdDSTCP, resTCP->ai_addr, resTCP->ai_addrlen);
    if (n == -1)
    {
        perror("[-] Failed to connect to TCP socket");
        failDSTCP();
    }
}

void showClientsSubscribedToGroup(char **tokenList, int numTokens)
{
    if (numTokens != 1)
    {
        fprintf(stderr, "[-] Incorrect list group's users command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_OUT)
    {
        fprintf(stderr, "[-] Please login before you request the list of users that are subscribed to your selected group.\n");
        return;
    }
    if (strlen(activeDSGID) == 0)
    {
        fprintf(stderr, "[-] Please select a group before you request the list of users that are subscribed to it.\n");
        return;
    }

    // Connect to DS TCP socket
    connectDSTCPSocket();

    // Send protocol message to DS
    char ulistClientMessage[CLIENTDS_ULISTBUF_SIZE];
    sprintf(ulistClientMessage, "ULS %s\n", activeDSGID);
    if (sendTCP(fdDSTCP, ulistClientMessage) == -1)
    {
        failDSTCP();
    }

    // Read message from DS -> indefinitely read until nl has been read
    int lenMsg = DSCLIENT_ULISTREAD_SIZE; // arbitrary initial size to read
    char *tmp = (char *)calloc(sizeof(char), lenMsg + 1);
    if (tmp == NULL)
    {
        fprintf(stderr, "[-] Failed to allocate memory in calloc.\n");
        closeTCPSocket(fdDSTCP, resTCP);
        return;
    }
    char *p_message = tmp;
    char readBuffer[DSCLIENT_ULISTREAD_SIZE];
    int n, bytesRead = 0;
    while ((n = readTCP(fdDSTCP, readBuffer, DSCLIENT_ULISTREAD_SIZE)) > 0)
    { // Read all the data that the DS sends
        if (bytesRead + n >= lenMsg)
        {
            char *new = (char *)realloc(p_message, 2 * lenMsg);
            if (new == NULL)
            {
                free(p_message);
                fprintf(stderr, "[-] Failed to allocate memory in calloc.\n");
                closeTCPSocket(fdDSTCP, resTCP);
                return;
            }
            // Set the new part to 0
            memset(new + lenMsg, 0, lenMsg);
            // Update pointer and len of buffer
            p_message = new;
            lenMsg *= 2;
        }
        memcpy(p_message + bytesRead, readBuffer, n);
        bytesRead += n;
    }
    if (p_message[bytesRead - 1] != '\n')
    { // Each request/reply ends with newline according to DS-Client communication protocol
        errDSTCP();
    }
    p_message[bytesRead - 1] = '\0';

    // We'll use another pointer so we can iterate over it and free properly the allocated memory
    char *p_aux = p_message;
    // Read status and code
    char codeDS[PROTOCOL_CODE_SIZE], statusDS[PROTOCOL_STATUS_TCP_SIZE];
    sscanf(p_aux, "%s %s", codeDS, statusDS);
    if (strcmp(codeDS, "RUL"))
    {
        errDSTCP();
    }

    // Parse response from received status
    if (!strcmp(statusDS, "OK"))
    {
        p_aux += 7; // len (RUL OK ) = 7
        char GName[DS_GNAME_SIZE], UID[CLIENT_UID_SIZE];
        sscanf(p_aux, "%s", GName);
        if (!validGName(GName))
        {
            free(tmp);
            errDSTCP();
        }
        p_aux += strlen(GName) + 1; // len(Gname) + len(' ')
        printf("[+] Users subscribed to %s: (UID)\n", GName);
        while (sscanf(p_aux, "%s ", UID) == 1)
        {
            if (!validUID(UID))
            {
                free(p_message);
                errDSTCP();
            }
            printf("-> %s\n", UID);
            p_aux += 6; // len (XXXXX ) = 6
        }
    }
    else if (!strcmp(statusDS, "NOK"))
    {
        fprintf(stderr, "[-] The group you've selected doesn't exist. Please try again.\n");
    }
    else
    { // Wrong protocol message received
        free(p_message);
        errDSTCP();
    }
    free(p_message);
    closeTCPSocket(fdDSTCP, resTCP);
}

void clientPostInGroup(char *command)
{
    if (clientSession == LOGGED_OUT)
    { // Client logged out
        fprintf(stderr, "[-] Please login before you post to a group.\n");
        return;
    }
    if (strlen(activeDSGID) == 0)
    { // No group selected
        fprintf(stderr, "[-] Please select a group before you post on it.\n");
        return;
    }
    connectDSTCPSocket();

    // Send message from client to the DS
    char messageText[PROTOCOL_TEXT_SIZE];
    char Fname[PROTOCOL_FNAME_SIZE] = "";
    sscanf(command, "post \"%240[^\"]\" %s", messageText, Fname); // makes sure that it only reads up to 240 characters
    if (strlen(Fname) > 0)
    { // fileName was 'filled' up with something -> there's a file to send
        if (!validFName(Fname))
        { // Validate the file sent
            fprintf(stderr, "[-] The file you submit can't exceed 24 characters and must have a 3 letter file extension. Please try again.\n");
            closeTCPSocket(fdDSTCP, resTCP);
            return;
        }

        // Extract its size in bytes
        FILE *post = fopen(Fname, "rb");
        if (post == NULL)
        {
            perror("[-] Error opening given file");
            closeTCPSocket(fdDSTCP, resTCP);
            return;
        }
        if (fseek(post, 0, SEEK_END) == -1)
        {
            perror("[-] Post file seek failed");
            failDSTCP();
        }
        long lenFile = ftell(post); // long because it can have at most 10 digits and int goes to 2^31 - 1 which is 214--.7 (len 10) - it can be 999 999 999 9 bytes
        if (lenFile == -1)
        {
            perror("[-] Post file tell failed");
            failDSTCP();
        }
        rewind(post);
        if (fclose(post) == -1)
        {
            perror("[-] Post file failed to close");
            failDSTCP();
        }

        // Send initial message
        char postMessageWFile[CLIENTDS_POSTWFILE_SIZE];
        sprintf(postMessageWFile, "PST %s %s %ld %s %s %ld ", activeClientUID, activeDSGID, strlen(messageText), messageText, Fname, lenFile);
        if (sendTCP(fdDSTCP, postMessageWFile) == -1)
        {
            failDSTCP();
        }

        // Send file
        if (sendFile(fdDSTCP, Fname, lenFile) == 0)
        {
            failDSTCP();
        }

        // Every reply/request must end with a \n
        char nl = '\n';
        if (sendTCP(fdDSTCP, &nl) == -1)
        {
            failDSTCP();
        }
    }
    else
    { // Text only case
        char postMessageWOFile[CLIENTDS_POSTWOFILE_SIZE];
        sprintf(postMessageWOFile, "PST %s %s %ld %s\n", activeClientUID, activeDSGID, strlen(messageText), messageText);
        if (sendTCP(fdDSTCP, postMessageWOFile) == -1)
        {
            failDSTCP();
        }
    }

    // Receive reply from the DS
    char postDSReply[DS_POSTREPLY_SIZE];
    int n;
    if ((n = readTCP(fdDSTCP, postDSReply, DS_POSTREPLY_SIZE - 1)) == -1)
    {
        failDSTCP();
    }
    if (postDSReply[n - 1] != '\n')
    { // Each request/reply ends with newline according to DS-Client communication protocol
        fprintf(stderr, "[-] Wrong protocol message received from server via TCP. Program will now exit.\n");
        failDSTCP();
    }
    postDSReply[n - 1] = '\0';

    // Process reply from the DS
    char codeDS[PROTOCOL_CODE_SIZE], statusDS[PROTOCOL_STATUS_TCP_SIZE];
    sscanf(postDSReply, "%s %s", codeDS, statusDS);
    if (strcmp(codeDS, "RPT"))
    { // Wrong protocol message received
        errDSTCP();
    }
    if (validMID(statusDS))
    {
        printf("[+] You have successfully posted in the selected group with message ID %s.\n", statusDS);
    }
    else if (!strcmp(statusDS, "NOK"))
    {
        fprintf(stderr, "[-] Failed to post in group. Please try again.\n");
    }
    else
    {
        errDSTCP();
    }
    closeTCPSocket(fdDSTCP, resTCP);
}

void clientRetrieveFromGroup(char **tokenList, int numTokens)
{
    if (numTokens != 2)
    { // R XXXX / RETRIEVE XXXX
        fprintf(stderr, "[-] Incorrect retrieve command usage. Please try again.\n");
        return;
    }
    if (clientSession == LOGGED_OUT)
    {
        fprintf(stderr, "[-] Please login before you retrieve messages from a group.\n");
        return;
    }
    if (strlen(activeDSGID) == 0)
    {
        fprintf(stderr, "[-] Please select a group before you retrieve messages from it.\n");
        return;
    }
    if (!validMID(tokenList[1]))
    {
        fprintf(stderr, "[-] Invalid starting message to retrieve. Please try again.\n");
        return;
    }

    connectDSTCPSocket();

    // Send message from client to the DS
    char retrieveMessageToDS[CLIENTDS_RTVBUF_SIZE];
    sprintf(retrieveMessageToDS, "RTV %s %s %s\n", activeClientUID, activeDSGID, tokenList[1]);
    if (sendTCP(fdDSTCP, retrieveMessageToDS) == -1)
    {
        failDSTCP();
    }

    // Receive retrieve reply from the DS
    char codeDS[PROTOCOL_CODE_SIZE + 1], statusDS[DSCLIENT_RTVSTATUS_SIZE + 1];
    char singleCharDS[CHAR_SIZE];
    int n;

    // Read the DS reply code
    if ((n = readTCP(fdDSTCP, codeDS, PROTOCOL_CODE_SIZE)) == -1)
    {
        failDSTCP();
    }
    codeDS[n - 1] = '\0'; // replace backspace with null terminator
    if (strcmp(codeDS, "RRT"))
    { // Wrong protocol message received
        errDSTCP();
    }

    // Read the DS reply status
    if ((n = readTCP(fdDSTCP, statusDS, DSCLIENT_RTVSTATUS_SIZE)) == -1)
    {
        failDSTCP();
    }
    statusDS[n] = '\0';
    if (!strcmp(statusDS, "EOF") || !strcmp(statusDS, "NOK"))
    { // Every reply/request must end with a \n
        if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
        {
            failDSTCP();
        }
        singleCharDS[n] = '\0';
        if (singleCharDS[n - 1] != '\n')
        {
            errDSTCP();
        }
    }

    // Process the DS reply
    if (!strcmp(statusDS, "EOF"))
    {
        printf("[+] There are no available messages to show in the selected group from the given starting message.\n");
        closeTCPSocket(fdDSTCP, resTCP);
        return;
    }
    if (!strcmp(statusDS, "NOK"))
    {
        fprintf(stderr, "[-] Failed to retrieve from group. Please check if you have a selected subscribed group and try again.\n");
        closeTCPSocket(fdDSTCP, resTCP);
        return;
    }

    // If it gets here then there are messages to retrieve
    char numMsgsBuf[DSCLIENT_RTVNMSG_SIZE + 1];
    if ((n = readTCP(fdDSTCP, numMsgsBuf, DSCLIENT_RTVNMSG_SIZE)) == -1)
    {
        failDSTCP();
    }
    numMsgsBuf[n] = '\0';
    int numMsgs = atoi(numMsgsBuf);
    if (numMsgs >= 10)
    { // If there are more than 10 messages we must read an extra space in the case where there are < 10 messages
        if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
        {
            failDSTCP();
        }
        singleCharDS[n] = '\0';
        if (singleCharDS[0] != ' ')
        { // There must be a space afterwards
            errDSTCP();
        }
    }

    // Display messages
    if (numMsgs == 1)
    {
        printf("[+] %d message to display: (-> MID: text \\(Fname - Fsize)):\n", numMsgs);
    }
    else
    {
        printf("[+] %d messages to display: (-> MID: text \\(Fname - Fsize)):\n", numMsgs);
    }
    int flagRTV = MID_OK;
    char MID[DS_MID_SIZE] = "", UID[CLIENT_UID_SIZE], TsizeBuf[DS_MSGTEXTSZ_SIZE] = "", Text[PROTOCOL_TEXT_SIZE + 1] = "";
    char FName[PROTOCOL_FNAME_SIZE] = "", FsizeBuf[PROTOCOL_FILESZ_SIZE] = "";
    int Tsize;
    long Fsize;
    int j;
    for (int i = 1; i <= numMsgs; ++i)
    {
        memset(MID, 0, sizeof(MID));
        memset(UID, 0, sizeof(UID));
        memset(TsizeBuf, 0, sizeof(TsizeBuf));
        memset(Text, 0, sizeof(Text));
        memset(FName, 0, sizeof(FName));
        memset(FsizeBuf, 0, sizeof(FsizeBuf));

        // Read MID
        if (flagRTV == MID_OK)
        { // If flag is MID_OK then we read a normal MID
            if ((n = readTCP(fdDSTCP, MID, DS_MID_SIZE)) == -1)
            { // RRTMID_SIZE to also read backspace
                failDSTCP();
            }
            MID[n - 1] = '\0';
            if (!validMID(MID))
            {
                errDSTCP();
            }
            printf("-> %s: ", MID);
        }
        else if (flagRTV == MID_CONCAT)
        { // If flag is MID_CONCAT then we must concat a character read previously to MID
            strcat(MID, singleCharDS);
            char *ptrMID = MID + 1;
            if ((n = readTCP(fdDSTCP, ptrMID, DS_MID_SIZE - 1)) == -1)
            { // RRTMID_SIZE-1 because discard contains first character and to also read backspace
                failDSTCP();
            }
            MID[n] = '\0';
            if (!validMID(MID))
            {
                errDSTCP();
            }
            printf("-> %s: ", MID);
        }

        // Read UID
        if ((n = readTCP(fdDSTCP, UID, CLIENT_UID_SIZE)) == -1)
        { // RRTMID_SIZE to also read backspace
            failDSTCP();
        }
        UID[n - 1] = '\0';
        if (!validUID(UID))
        {
            errDSTCP();
        }

        // Read text size
        for (j = 0; j < DS_MSGTEXTSZ_SIZE; ++j)
        {
            if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
            {
                failDSTCP();
            }
            if (singleCharDS[0] != ' ')
            { // Anything other than the space we'll append or new line if retrieve message ends with text
                strcat(TsizeBuf, &singleCharDS[0]);
            }
            else
            { // Backpace has been read
                break;
            }
        }
        TsizeBuf[j] = '\0';
        if (singleCharDS[0] != ' ' || !isNumber(TsizeBuf) || atoi(TsizeBuf) > 240)
        { // Make sure space was read and Tsize is a number - otherwise wrong protocol message received
            errDSTCP();
        }

        // Read text
        Tsize = atoi(TsizeBuf);
        if ((n = readTCP(fdDSTCP, Text, Tsize + 1)) == -1)
        {
            failDSTCP();
        }
        Text[n] = '\0';
        if ((Text[Tsize] != ' ') && (Text[Tsize] != '\n'))
        { // Wrong protocol message received -> either it ends or has a file
            errDSTCP();
        }
        if (Text[Tsize] == '\n')
        { // End of reply has been made - remove new line on printing
            Text[Tsize] = '\0';
            printf("%s\n", Text);
            break;
        }
        printf("%s\n", Text);
        // Read next character -> either it's a slash(/) or the first char of a MID
        if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
        {
            failDSTCP();
        }
        singleCharDS[n] = '\0';
        if (isNumber(&singleCharDS[0]))
        { // it read the first digit of the next MID
            flagRTV = MID_CONCAT;
        }
        else if (singleCharDS[0] == '/')
        { // there's a file to be read
            flagRTV = MID_OK;

            // Read backspace
            if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
            {
                failDSTCP();
            }
            singleCharDS[n] = '\0';
            if (singleCharDS[0] != ' ')
            {
                errDSTCP();
            }

            // Read filename
            for (j = 0; j < PROTOCOL_FNAME_SIZE; ++j)
            {
                if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
                {
                    failDSTCP();
                }
                if (singleCharDS[0] != ' ')
                { // Anything other than the space we'll append
                    strcat(FName, &singleCharDS[0]);
                }
                else
                { // Space has been read
                    break;
                }
            }
            FName[j] = '\0';
            if (singleCharDS[0] != ' ' || !validFName(FName))
            {
                errDSTCP();
            }
            printf("(%s - ", FName);

            // Read file size
            for (j = 0; j < PROTOCOL_FILESZ_SIZE; ++j)
            {
                if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
                {
                    failDSTCP();
                }
                if (singleCharDS[0] != ' ')
                { // Anything other than the space we'll append
                    strcat(FsizeBuf, &singleCharDS[0]);
                }
                else
                { // Space has been read
                    break;
                }
            }
            FsizeBuf[j] = '\0';
            printf("%s bytes)\n", FsizeBuf);
            if (singleCharDS[0] != ' ')
            {
                errDSTCP();
            }
            Fsize = atol(FsizeBuf);
            if (recvFile(fdDSTCP, FName, Fsize) == 0)
            {
                failDSTCP();
            }
            if ((n = readTCP(fdDSTCP, singleCharDS, CHAR_SIZE - 1)) == -1)
            {
                failDSTCP();
            }
            singleCharDS[n] = '\0';
            if (singleCharDS[0] != ' ' && singleCharDS[0] != '\n')
            { // Read extra space in file or last message \n
                errDSTCP();
            }
            if (i == numMsgs && singleCharDS[0] != '\n')
            { // Every reply/request must end with a \n
                errDSTCP();
            }
        }
    }

    // After receiving the messsages the client sends the DS a confirmation
    if (sendTCP(fdDSTCP, "OK\n") == -1)
    {
        failDSTCP();
    }
    closeTCPSocket(fdDSTCP, resTCP);
}