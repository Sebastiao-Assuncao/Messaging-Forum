#ifndef DS_UDPANDTCP_H
#define DS_UDPANDTCP_H

#include "../../centralizedmsg-api-constants.h"
#include "../../centralizedmsg-api.h"
#include "../centralizedmsg-server-api.h"

extern char portDS[DS_PORT_SIZE];
extern int verbose;

/**
 * @brief Create all the DS related sockets (UDP and TCP protocol).
 *
 */
void setupDSSockets();

/**
 * @brief Logs what the client sent to the DS in STDOUT.
 *
 * @param clientBuf string that contains the message that the client sent.
 * @param s struct that contains the socket address information.
 */
void logVerbose(char *clientBuf, struct sockaddr_in s);

/**
 * @brief Handle all messages exchange between the client and the DS via UDP protocol.
 *
 */
void handleDSUDP();

/**
 * @brief Handle all messages exchange between the client and the DS via TCP protocol.
 *
 */
void handleDSTCP();

#endif