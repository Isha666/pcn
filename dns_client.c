
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080
#define CACHE_SIZE 5

// Client cache
char cacheDomain[CACHE_SIZE][100];
char cacheIP[CACHE_SIZE][100];

int cacheCount = 0;

// Check cache
int checkCache(char domain[], char ip[])
{
    for (int i = 0; i < cacheCount; i++)
    {
        if (strcmp(cacheDomain[i], domain) == 0)
        {
            strcpy(ip, cacheIP[i]);
            return 1;
        }
    }

    return 0;
}

// Add to cache
void addCache(char domain[], char ip[])
{
    if (cacheCount < CACHE_SIZE)
    {
        strcpy(cacheDomain[cacheCount], domain);
        strcpy(cacheIP[cacheCount], ip);

        cacheCount++;
    }
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t serverSize;

    char domain[100];
    char response[100];

    // Create UDP socket
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    // Server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    serverSize = sizeof(serverAddr);

    // Set timeout = 5 seconds
    struct timeval timer;

    timer.tv_sec = 5;
    timer.tv_usec = 0;

    setsockopt(clientSocket,
               SOL_SOCKET,
               SO_RCVTIMEO,
               &timer,
               sizeof(timer));

    while (1)
    {
        printf("\nEnter domain name (or 'exit'): ");
        scanf("%99s", domain);

        if (strcmp(domain, "exit") == 0)
            break;

        // Check cache first
        if (checkCache(domain, response))
        {
            printf("CACHE HIT\n");
            printf("IP Address: %s\n", response);
            continue;
        }

        printf("CACHE MISS\n");

        // Send DNS query
        sendto(clientSocket,
               domain,
               strlen(domain) + 1,
               0,
               (struct sockaddr *)&serverAddr,
               serverSize);

        // Receive DNS response
        int n = recvfrom(clientSocket,
                         response,
                         sizeof(response) - 1,
                         0,
                         (struct sockaddr *)&serverAddr,
                         &serverSize);

        // Timeout
        if (n < 0)
        {
            printf("TIMEOUT: DNS server did not respond.\n");
            continue;
        }

        response[n] = '\0';

        // Check errors
        if (strcmp(response, "NXDOMAIN") == 0)
        {
            printf("NXDOMAIN: Domain does not exist.\n");
        }
        else if (strcmp(response, "INVALID FORMAT") == 0)
        {
            printf("ERROR: Invalid domain format.\n");
        }
        else
        {
            printf("IP Address: %s\n", response);

            // Store result in cache
            addCache(domain, response);
        }
    }

    close(clientSocket);

    return 0;
}
