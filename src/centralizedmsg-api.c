#include "centralizedmsg-api.h"
#include "centralizedmsg-api-constants.h"

#include <regex.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y)) // Macro to determine min(x, y)

int validRegex(char *buf, char *reg)
{
    int reti;
    regex_t regex;
    reti = regcomp(&regex, reg, REG_EXTENDED);
    if (reti)
    { // If the regex didn't compile
        fprintf(stderr, "[-] Internal error on parsing regex. Please try again later and/or contact the developers.\n");
        return 0;
    }
    if (regexec(&regex, buf, (size_t)0, NULL, 0))
    { // If the buffer doesn't match with the pattern
        regfree(&regex);
        return 0;
    }
    regfree(&regex); // Free allocated memory
    return 1;
}

int validAddress(char *address)
{ // First pattern is for hostname and second one is for IPv4 addresses/IP addresses
    return (validRegex(address, "^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9-]*[a-zA-Z0-9]).)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9])$") ||
            validRegex(address, "^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$"));
}

int validPort(char *port)
{ // Ports range from 0 to 65535
    return validRegex(port, "^([0-9]{1,4}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
}

int parseClientDSCommand(char *command)
{
    if (!strcmp(command, "reg"))
        return REGISTER;
    else if (!strcmp(command, "unregister") || !strcmp(command, "unr"))
        return UNREGISTER;
    else if (!strcmp(command, "login"))
        return LOGIN;
    else if (!strcmp(command, "logout"))
        return LOGOUT;
    else if (!strcmp(command, "showuid") || !strcmp(command, "su"))
        return SHOWUID;
    else if (!strcmp(command, "exit"))
        return EXIT;
    else if (!strcmp(command, "groups") || !strcmp(command, "gl"))
        return GROUPS;
    else if (!strcmp(command, "subscribe") || !strcmp(command, "s"))
        return SUBSCRIBE;
    else if (!strcmp(command, "unsubscribe") || !strcmp(command, "u"))
        return UNSUBSCRIBE;
    else if (!strcmp(command, "my_groups") || !strcmp(command, "mgl"))
        return MY_GROUPS;
    else if (!strcmp(command, "select") || !strcmp(command, "sag"))
        return SELECT;
    else if (!strcmp(command, "showgid") || !strcmp(command, "sg"))
        return SHOWGID;
    else if (!strcmp(command, "ulist") || !strcmp(command, "ul"))
        return ULIST;
    else if (!strcmp(command, "post"))
        return POST;
    else if (!strcmp(command, "retrieve") || !strcmp(command, "r"))
        return RETRIEVE;
    else
    { // No valid command was received
        fprintf(stderr, "[-] Invalid user command code. Please try again.\n");
        return INVALID_COMMAND;
    }
}

int validUID(char *UID)
{
    return validRegex(UID, "^[0-9]{5}$");
}

int validPW(char *PW)
{
    return validRegex(PW, "^[a-zA-Z0-9]{8}$");
}

int isNumber(char *num)
{
    size_t len = strlen(num);
    for (int i = 0; i < len; ++i)
    {
        if (num[i] < '0' || num[i] > '9')
            return 0;
    }
    return 1;
}

int validGID(char *GID)
{
    return validRegex(GID, "^([0][1-9]|[1-9][0-9])$");
}

int validGName(char *GName)
{
    return validRegex(GName, "^[a-zA-Z0-9_-]{1,24}$");
}

int isMID(char *MID)
{
    return validRegex(MID, "^[0-9]{4}$");
}

int sendTCP(int fd, char *message)
{
    int bytesSent = 0;
    ssize_t nSent;
    size_t messageLen = strlen(message);
    while (bytesSent < messageLen)
    { // Send initial message
        nSent = write(fd, message + bytesSent, messageLen - bytesSent);
        if (nSent == -1)
        {
            perror("[-] Failed to write on TCP");
            return nSent;
        }
        bytesSent += nSent;
    }
    return bytesSent;
}

int readTCP(int fd, char *message, int maxSize)
{
    int bytesRead = 0;
    ssize_t n;

    while (bytesRead < maxSize)
    {
        if (timerOn(fd) == -1)
        {
            perror("[-] Failed to start TCP timer");
            return -1;
        }
        n = read(fd, message + bytesRead, maxSize - bytesRead);

        // As soon as we read the first byte, turn off timer
        if (timerOff(fd) == -1)
        {
            perror("[-] Failed to turn off TCP timer");
            return -1;
        }

        if (n == 0)
        {
            break; // Peer has performed an orderly shutdown -> POSSIBLE message complete
        }
        if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                perror("[-] TCP socket timed out while reading. Program will now exit.\n");
                return 0;
            }
            else
            {
                perror("[-] Failed to receive from server on TCP");
                return n;
            }
        }
        bytesRead += n;
    }
    return bytesRead;
}

int validFName(char *FName)
{
    return validRegex(FName, "^[a-zA-Z0-9_-]{1,20}[.]{1}[a-zA-Z0-9]{3}$");
}

int validMID(char *MID)
{
    if (!strcmp(MID, "0000"))
        return 0;
    return validRegex(MID, "^[0-9]{0,4}$");
}

/**
 * @brief Auxiliary function of sendFile that sends an unsigned char buffer via TCP protocol.
 *
 * @param fd file descriptor to send the data to.
 * @param buffer string that contains the buffer to be sent.
 * @param num number of bytes to be sent.
 * @return 1 if num bytes of buffer were sent, 0 otherwise.
 */
static int sendData(int fd, unsigned char *buffer, size_t num)
{
    unsigned char *tmpBuf = buffer;
    ssize_t n;
    while (num > 0)
    {
        n = write(fd, tmpBuf, num);
        if (n == -1)
        {
            perror("[-] Failed to send file data via TCP");
            return 0;
        }
        tmpBuf += n;
        num -= n;
    }
    return 1;
}

int sendFile(int fd, char *filePath, long lenFile)
{
    FILE *post = fopen(filePath, "rb");
    if (post == NULL)
    {
        return 0;
    }
    unsigned char buffer[FILEBUFFER_SIZE];
    do
    {
        size_t num = MIN(lenFile, FILEBUFFER_SIZE);
        num = fread(buffer, sizeof(unsigned char), num, post);
        if (num < 1)
        {
            fprintf(stderr, "[-] Failed on reading the given file. Please try again.\n");
            fclose(post);
            return 0;
        }
        if (!sendData(fd, buffer, num))
        {
            fclose(post);
            return 0;
        }
        lenFile -= num;
        memset(buffer, 0, sizeof(buffer));
    } while (lenFile > 0);
    if (fclose(post) == -1)
    {
        return 0;
    }
    return 1;
}

int recvFile(int fd, char *FName, long Fsize)
{
    long bytesRecv = 0;
    int toRead;
    unsigned char bufFile[FILEBUFFER_SIZE] = "";
    ssize_t n;
    FILE *file = fopen(FName, "wb");

    if (!file)
    {
        perror("Failed to create file");
        return 0;
    }

    do
    {
        if (timerOn(fd) == -1)
        {
            perror("[-] Failed to start TCP timer");
            return 0;
        }
        toRead = MIN(sizeof(bufFile), Fsize - bytesRecv);
        n = read(fd, bufFile, toRead);

        if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                perror("[-] TCP socket timed out while reading. Program will now exit.\n");
                return 0;
            }
            else
            {
                perror("[-] Failed to read from TCP");
                fclose(file);
                return 0;
            }
        }

        if (timerOff(fd) == -1)
        {
            perror("[-] Failed to turn off TCP timer");
            return 0;
        }

        if (n > 0)
        {
            bytesRecv += n;
            if (fwrite(bufFile, sizeof(unsigned char), n, file) != n)
            {
                fprintf(stderr, "[-] Failed to write on file.\n");
                fclose(file);
                return 0;
            }
        }
        memset(bufFile, 0, n);
    } while (bytesRecv < Fsize);
    if (fclose(file) == -1)
    {
        fprintf(stderr, "[-] Failed to close file.\n");
        return 0;
    }
    return 1;
}

void closeUDPSocket(int fdUDP, struct addrinfo *resUDP)
{
    freeaddrinfo(resUDP);
    close(fdUDP);
}

void closeTCPSocket(int fdTCP, struct addrinfo *resTCP)
{
    freeaddrinfo(resTCP);
    close(fdTCP);
}

int parseDSClientCommand(char *command)
{
    if (!strcmp(command, "REG"))
        return REGISTER;
    else if (!strcmp(command, "UNR"))
        return UNREGISTER;
    else if (!strcmp(command, "LOG"))
        return LOGIN;
    else if (!strcmp(command, "OUT"))
        return LOGOUT;
    else if (!strcmp(command, "GLS"))
        return GROUPS;
    else if (!strcmp(command, "GSR"))
        return SUBSCRIBE;
    else if (!strcmp(command, "GUR"))
        return UNSUBSCRIBE;
    else if (!strcmp(command, "GLM"))
        return MY_GROUPS;
    else if (!strcmp(command, "ULS"))
        return ULIST;
    else if (!strcmp(command, "PST"))
        return POST;
    else if (!strcmp(command, "RTV"))
        return RETRIEVE;
    else
    { // No valid command was received
        fprintf(stderr, "[-] Invalid user command code.\n");
        return INVALID_COMMAND;
    }
}

int isGID(char *GID)
{
    return validRegex(GID, "^[0-9]{2}");
}

int timerOn(int fd)
{
    struct timeval timeout;
    memset((char *)&timeout, 0, sizeof(timeout));
    timeout.tv_sec = 3;
    return (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeout, sizeof(struct timeval)));
}

int timerOff(int fd)
{
    struct timeval timeout;
    memset((char *)&timeout, 0, sizeof(timeout));
    return (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&timeout, sizeof(struct timeval)));
}