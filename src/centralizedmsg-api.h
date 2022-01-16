#ifndef API_H
#define API_H

#include "centralizedmsg-api-constants.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * @brief Checks if a given buffer specifies a given pattern.
 *
 * @param buffer buffer to be checked.
 * @param pattern pattern that buffer is being checked on.
 * @return 1 if buffer specifies the given pattern, 0 otherwise.
 */
int validRegex(char *buffer, char *pattern);

/**
 * @brief Checks if a given address/hostname is valid.
 *
 * @param address string containing the address.
 * @return 1 if it's valid, 0 otherwise.
 */
int validAddress(char *address);

/**
 * @brief Checks if a given port is valid.
 *
 * @param portStr string containing the port.
 * @return 1 if it's valid, 0 otherwise.
 */
int validPort(char *port);

/**
 * @brief "Translates" a given command into a pre-defined macro.
 *
 * @param command string containing the command.
 * @return respective MACRO assigned to given command.
 */
int parseClientDSCommand(char *command);

/**
 * @brief Checks if a given user ID is valid according to the statement's rules.
 *
 * @param UID string that contains the user ID.
 * @return 1 if it's valid, 0 otherwise.
 */
int validUID(char *UID);

/**
 * @brief Checks if a given user password is valid according to the statement's rules.
 *
 * @param PW string that contains the password.
 * @return 1 if it's valid, 0 otherwise.
 */
int validPW(char *PW);

/**
 * @brief Checks if a given string is a decimal positive number.
 *
 * @param num string that contains the number (or not).
 * @return 1 if it's a decimal positive number, 0 otherwise.
 */
int isNumber(char *number);

/**
 * @brief Checks if a given group ID is valid according to the statement's rules.
 *
 * @param GID string that contains the group ID.
 * @return 1 if it's valid, 0 otherwise.
 */
int validGID(char *GID);

/**
 * @brief Checks if a given group name is valid according to the statement's rules.
 *
 * @param gName string that contains the group name.
 * @return 1 if it's valid, 0 otherwise.
 */
int validGName(char *GName);

/**
 * @brief Checks if a given group message ID is according to the statement's rules.
 *
 * @param MID string that contains the group message ID.
 * @return 1 if 0000 <= MID <= 9999, 0 otherwise.
 */
int isMID(char *MID);

/**
 * @brief Sends a message to a specified file descriptor via TCP protocol.
 *
 * @param message string that contains the message to be sent.
 * @return number of bytes sent if successful, -1 otherwise.
 */
int sendTCP(int fd, char *message);

/**
 * @brief Reads from a file descriptor on to a buffer via TCP protocol.
 *
 * @param fd file descriptor to read via TCP protocol.
 * @param message buffer to store what is read.
 * @param maxSize maximum number of bytes to read at a time.
 * @return -1 if read failed, otherwise the total number of bytes read.
 */
int readTCP(int fd, char *message, int maxSize);

/**
 * @brief Checks if a given file name is valid according to the statement's rules.
 *
 * @param FName string that contains the file name.
 * @return 1 if it's valid, 0 otherwise.
 */
int validFName(char *FName);

/**
 * @brief Checks if a given group message ID is valid according to the statement's rules.
 *
 * @param MID string that contains the message ID.
 * @return 1 if 0001 <= MID <= 9999, 0 otherwise.
 */
int validMID(char *MID);

/**
 * @brief Sends a file via TCP.
 *
 * @param fd file descriptor to send the data to.
 * @param post file stream of the file being sent-
 * @param lenFile number of bytes in file being sent.
 * @return 1 if file was sent, 0 otherwise.
 */
int sendFile(int fd, char *filePath, long lenFile);

/**
 * @brief Receives a file via TCP.
 *
 * @param fd file descriptor to read the data from.
 * @param FName name of the file being received.
 * @param Fsize number
 * @return 1 if file was received, 0 otherwise.
 */
int recvFile(int fd, char *FName, long Fsize);

/**
 * @brief Closes the socket created to exchange messages between the client and the DS via UDP protocol.
 *
 */
void closeUDPSocket(int fdUDP, struct addrinfo *resUDP);

/**
 * @brief Closes the socket created to exchange messages between the client and the DS via TCP protocol.
 *
 */
void closeTCPSocket(int fdTCP, struct addrinfo *resTCP);

/**
 * @brief "Translates" a given protocol message code into a pre-defined macro.
 *
 * @param command string containing the protocol message code.
 * @return respective MACRO assigned to given message code.
 */
int parseDSClientCommand(char *command);

/**
 * @brief Checks if a given group ID is valid according to the statement's rules.
 *
 * @param UID string that contains the group ID.
 * @return 1 if it's valid, 0 otherwise.
 */
int isGID(char *GID);

/**
 * @brief Sets a timeout to read data on the given file descriptor.
 *
 * @param fd file descriptor that will be set a read timeout.
 * @return result of setsockopt().
 */
int timerOn(int fd);

/**
 * @brief Turns a timeout associated to a file descriptor off.
 *
 * @param fd file descriptor that will reset from a read timeout.
 * @return result of setsockopt().
 */
int timerOff(int fd);

#endif