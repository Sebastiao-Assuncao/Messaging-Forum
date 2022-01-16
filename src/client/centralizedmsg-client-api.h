#ifndef CLIENTAPI_H
#define CLIENTAPI_H

#include "../centralizedmsg-api.h"
#include "../centralizedmsg-api-constants.h"

extern char addrDS[DS_ADDR_SIZE];
extern char portDS[DS_PORT_SIZE];

/**
 * @brief Creates socket that enables client-server communication via UDP protocol.
 *
 */
void createDSUDPSocket();

/**
 * @brief Exchanges messages between the client and the DS via UDP protocol.
 *
 * @param message string that contains the message that client wants to send to the DS.
 */
void exchangeDSUDPMsg(char *message);

/**
 * @brief Processes the DS's reply to what the user sent it via UDP protocol.
 *
 * @param message string that contains the message that the DS sent to the client.
 */
void processDSUDPReply(char *message);

/**
 * @brief Creates a new account on the DS exchanging messages via UDP protocol.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientRegister(char **tokenList, int numTokens);

/**
 * @brief Deletes an existing account on the DS exchanging messages via UDP protocol.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientUnregister(char **tokenList, int numTokens);

/**
 * @brief Logs a user in to an existing DS account exchanging messages via UDP protocol.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientLogin(char **tokenList, int numTokens);

/**
 * @brief Logs a user out from an existing DS account exchanging messages via UDP protocol.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientLogout(char **tokenList, int numTokens);

/**
 * @brief Locally displays the current logged in user ID.
 *
 * @param numTokens number of command arguments.
 */
void showCurrentClient(int numTokens);

/**
 * @brief Terminates the client program gracefully.
 *
 * @param numTokens number of command arguments.
 */
void clientExit(int numTokens);

/**
 * @brief Displays all the existing groups in the DS.
 *
 * @param numTokens number of command arguments.
 */
void showDSGroups(int numTokens);

/**
 * @brief Enables the current client to either create a new group in the DS or to subscribe to one.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientSubscribeGroup(char **tokenList, int numTokens);

/**
 * @brief Unsubscribes the current client to the given group ID.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientUnsubscribeGroup(char **tokenList, int numTokens);

/**
 * @brief Displays all the groups in the DS that the current logged in client is subscribed to.
 *
 * @param numTokens number of command arguments.
 */
void clientShowSubscribedGroups(int numTokens);

/**
 * @brief Locally selects a group to use ulist, post and retrieve on a DS group.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientSelectGroup(char **tokenList, int numTokens);

/**
 * @brief Locally displays the selected DS group's ID.
 *
 * @param numTokens
 */
void showCurrentSelectedGID(int numTokens);

/**
 * @brief Estabelish a connection via TCP protocol between the client and the DS.
 *
 */
void connectDSTCPSocket();

/**
 * @brief Shows all users that are subscribed to the current selected DS group.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void showClientsSubscribedToGroup(char **tokenList, int numTokens);

/**
 * @brief Posts a message on the current selected DS group.
 *
 * @param message string that contains the input command given to stdin.
 */
void clientPostInGroup(char *message);

/**
 * @brief Shows all messages from the current selected DS group starting from the given message ID.
 *
 * @param tokenList list that contains all the command's arguments (including the command itself).
 * @param numTokens number of command arguments.
 */
void clientRetrieveFromGroup(char **tokenList, int numTokens);

#endif