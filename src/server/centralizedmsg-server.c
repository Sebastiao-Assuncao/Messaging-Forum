#include "centralizedmsg-server-api.h"
#include "ds-api/ds-udpandtcp.h"
#include "../centralizedmsg-api.h"
#include "../centralizedmsg-api-constants.h"
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Parses the program's arguments for the DS port and verbose mode.
 *
 * @param argc number of arguments (including the executable) given.
 * @param argv arguments given.
 */
static void parseArgs(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    parseArgs(argc, argv);
    setupDSSockets();
    fillDSGroupsInfo();
    // Have 2 separate processes handling different operations
    pid_t pid = fork();
    if (pid == 0)
    { // Set child process to handle UDP operations
        handleDSTCP();
    }
    else if (pid > 0)
    { // Set parent process to handle TCP operations
        handleDSUDP();
    }
    else
    {
        perror("[-] Failed to fork");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

static void parseArgs(int argc, char *argv[])
{
    if (!(argc == 1 || argc == 2 || argc == 3 || argc == 4))
    {
        fprintf(stderr, "[-] Invalid client program arguments. Usage: ./DS [-p DSport] [-v]\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 1; i <= argc - 1; ++i)
    {
        if (argv[i][0] != '-')
        { // If it's not a flag ignore
            continue;
        }
        switch (argv[i][1])
        {
        case 'p':
            if (validPort(argv[i + 1]))
            {
                strcpy(portDS, argv[i + 1]);
            }
            else
            {
                fprintf(stderr, "[-] Invalid DS port number given. Please try again.\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'v':
            if (argc == 2 || argc == 4)
            {
                verbose = VERBOSE_ON;
            }
            else
            {
                fprintf(stderr, "[-] Verbose mode doesn't take any arguments. Usage: ./DS [-p DSport] [-v]\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            fprintf(stderr, "[-] Invalid flag given. Usage: ./DS [-p DSport] [-v]\n");
            exit(EXIT_FAILURE);
        }
    }
}