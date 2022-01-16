#include "ds-udpandtcp.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* DS Server information variables */
char portDS[DS_PORT_SIZE] = DS_DEFAULT_PORT;
int verbose = VERBOSE_OFF;

/* UDP Socket related variables */
int fdDSUDP;
struct addrinfo hintsUDP, *resUDP;

/* TCP Socket related variables */
int listenTCPDS;
struct addrinfo hintsTCP, *resTCP;

void setupDSSockets()
{
    // UDP
    fdDSUDP = socket(AF_INET, SOCK_DGRAM, 0);
    if (fdDSUDP == -1)
    {
        perror("[-] Server UDP socket failed to create");
        exit(EXIT_FAILURE);
    }
    memset(&hintsUDP, 0, sizeof(hintsUDP));
    hintsUDP.ai_family = AF_INET;
    hintsUDP.ai_socktype = SOCK_DGRAM;
    hintsUDP.ai_flags = AI_PASSIVE;
    int errcode = getaddrinfo(NULL, portDS, &hintsUDP, &resUDP);
    if (errcode != 0)
    {
        perror("[-] Failed on UDP address translation");
        close(fdDSUDP);
        exit(EXIT_FAILURE);
    }
    int n = bind(fdDSUDP, resUDP->ai_addr, resUDP->ai_addrlen);
    if (n == -1)
    {
        perror("[-] Failed to bind UDP server");
        closeUDPSocket(fdDSUDP, resUDP);
        exit(EXIT_FAILURE);
    }

    // TCP
    listenTCPDS = socket(AF_INET, SOCK_STREAM, 0);
    if (listenTCPDS == -1)
    {
        perror("[-] Server TCP socket failed to create");
        closeUDPSocket(fdDSUDP, resUDP);
        exit(EXIT_FAILURE);
    }
    memset(&hintsTCP, 0, sizeof(hintsTCP));
    hintsTCP.ai_family = AF_INET;
    hintsTCP.ai_socktype = SOCK_STREAM;
    hintsTCP.ai_flags = AI_PASSIVE;
    int err;
    err = getaddrinfo(NULL, portDS, &hintsTCP, &resTCP);
    if (err != 0)
    {
        perror("[-] Failed on TCP address translation");
        closeUDPSocket(fdDSUDP, resUDP);
        close(listenTCPDS);
        exit(EXIT_FAILURE);
    }
    err = bind(listenTCPDS, resTCP->ai_addr, resTCP->ai_addrlen);
    if (err == 1)
    {
        perror("[-] Server TCP socket failed to bind");
        closeUDPSocket(fdDSUDP, resUDP);
        closeTCPSocket(listenTCPDS, resTCP);
        exit(EXIT_FAILURE);
    }
    err = listen(listenTCPDS, DS_LISTENQUEUE_SIZE);
    if (err == -1)
    {
        perror("[-] Failed to prepare TCP socket to accept connections");
        closeUDPSocket(fdDSUDP, resUDP);
        closeTCPSocket(listenTCPDS, resTCP);
        exit(EXIT_FAILURE);
    }

    // If verbose is on print out where the DS server is running and at which port it's listening to
    if (verbose == VERBOSE_ON)
    {
        char hostname[DS_HOSTNAME_SIZE + 1];
        if (gethostname(hostname, DS_HOSTNAME_SIZE) == -1)
        {
            fprintf(stderr, "[-] Failed to get DS hostname.\n");
            closeUDPSocket(fdDSUDP, resUDP);
            closeTCPSocket(listenTCPDS, resTCP);
            exit(EXIT_FAILURE);
        }
        printf("[+] DS server started @ %s.\n[!] Currently listening in port %s for UDP and TCP connections...\n\n", hostname, portDS);
    }
}

void logVerbose(char *clientBuf, struct sockaddr_in s)
{
    printf("[!] Client @ %s in port %d sent: %s\n", inet_ntoa(s.sin_addr), ntohs(s.sin_port), clientBuf);
}

void handleDSUDP()
{
    char clientBuf[CLIENT_TO_DS_UDP_SIZE];
    char *serverBuf;
    struct sockaddr_in cliaddr;
    socklen_t addrlen;
    ssize_t n;
    while (1)
    {
        addrlen = sizeof(cliaddr);
        n = recvfrom(fdDSUDP, clientBuf, sizeof(clientBuf), 0, (struct sockaddr *)&cliaddr, &addrlen);
        if (n == -1)
        {
            perror("[-] (UDP) DS failed on recvfrom");
            closeUDPSocket(fdDSUDP, resUDP);
            exit(EXIT_FAILURE);
        }
        if (clientBuf[n - 1] != '\n')
        { // Every request/reply must end with a newline \n
            n = sendto(fdDSUDP, ERR_MSG, ERR_MSG_SIZE, 0, (struct sockaddr *)&cliaddr, addrlen);
            if (n == -1)
            {
                perror("[-] (UDP) DS failed on sendto");
                closeUDPSocket(fdDSUDP, resUDP);
                exit(EXIT_FAILURE);
            }
        }
        clientBuf[n - 1] = '\0';
        if (verbose == VERBOSE_ON)
        {
            logVerbose(clientBuf, cliaddr);
        }
        serverBuf = processClientUDP(clientBuf);
        n = sendto(fdDSUDP, serverBuf, strlen(serverBuf), 0, (struct sockaddr *)&cliaddr, addrlen);
        if (n == -1)
        {
            perror("[-] (UDP) DS failed on sendto");
            free(serverBuf);
            closeUDPSocket(fdDSUDP, resUDP);
            exit(EXIT_FAILURE);
        }
        free(serverBuf);
    }
}

void handleDSTCP()
{
    struct sockaddr_in cliaddr;
    socklen_t addrlen;
    int newDSFDTCP;
    pid_t pid;
    while (1)
    {
        addrlen = sizeof(cliaddr);
        if ((newDSFDTCP = accept(listenTCPDS, (struct sockaddr *)&cliaddr, &addrlen)) == -1)
        { // If this connect failed to accept let's continue to try to look for new ones
            continue;
        }
        if ((pid = fork()) == 0)
        {
            close(listenTCPDS);
            char commandCode[PROTOCOL_CODE_SIZE];
            int n = readTCP(newDSFDTCP, commandCode, PROTOCOL_CODE_SIZE);
            if (n == -1)
            { // TODO: CHECK TIMEOUT
                close(newDSFDTCP);
                exit(EXIT_FAILURE);
            }
            if (commandCode[PROTOCOL_CODE_SIZE - 1] != ' ')
            { // Protocol is (XXX )
                sendTCP(newDSFDTCP, ERR_MSG);
                close(newDSFDTCP);
                exit(EXIT_FAILURE);
            }
            commandCode[PROTOCOL_CODE_SIZE - 1] = '\0'; // Remove backspace
            // TODO: ADD VERBOSE
            processClientTCP(newDSFDTCP, commandCode);
            close(newDSFDTCP);
            exit(EXIT_SUCCESS);
        }
        close(newDSFDTCP);
    }
}