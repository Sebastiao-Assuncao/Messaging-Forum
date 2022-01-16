#ifndef SERVERAPI_H
#define SERVERAPI_H

#include "ds-api/ds-operations.h"
#include "../centralizedmsg-api.h"
#include "../centralizedmsg-api-constants.h"

/**
 * @brief Process and exchange of messages between the client and the DS via UDP protocol.
 *
 * @param message string that contains the buffer the client sent to the DS.
 * @return char* string that contains the buffer that the DS will send to the client.
 */
char *processClientUDP(char *message);

/**
 * @brief Process and exchange of messages between the client and the DS via TCP protocol.
 *
 * @param fd file descriptor where the TCP connection was made.
 * @param command string that contains the TCP operation code according to the statement's rules.
 */
void processClientTCP(int fd, char *command);

/**
 * @brief Registers a client in the DS.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *clientRegister(char **tokenList, int numTokens);

/**
 * @brief Unregisters a client from the DS.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *clientUnregister(char **tokenList, int numTokens);

/**
 * @brief Logs a client in to the DS.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *clientLogin(char **tokenList, int numTokens);

/**
 * @brief Logs a client out from the DS.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *clientLogout(char **tokenList, int numTokens);

/**
 * @brief Lists all existing DS groups.
 *
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *listDSGroups(int numTokens);

/**
 * @brief Subscribes a client to an existing DS group or creates a new one.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *clientSubscribeGroup(char **tokenList, int numTokens);

/**
 * @brief Unsubscribes a client from an existing DS group.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *clientUnsubscribeGroup(char **tokenList, int numTokens);

/**
 * @brief Lists all groups that a given client is subscribed to.
 *
 * @param tokenList list that contains all the protocol message's arguments (including the message code).
 * @param numTokens number of command arguments.
 * @return char* containing the DS reply to the client.
 */
char *listClientDSGroups(char **tokenList, int numTokens);

/**
 * @brief Lists all clients that are subscribed to a selected client group.
 *
 * @param fd file descriptor where the TCP connection was made to request this command.
 */
void showClientsInGroup(int fd);

/**
 * @brief Post client message in a DS group.
 *
 * @param fd file descriptor where the TCP connection was made to request this command.
 */
void clientPostInGroup(int fd);

/**
 * @brief Retrieves N (N <= 20) messages from a given DS group.
 *
 * @param fd file descriptor where the TCP connection was made to request this command.
 */
void retrieveMessagesFromGroup(int fd);

#endif