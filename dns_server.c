
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

// DNS database
char *domains[] = {
    "google.com",
    "youtube.com",
    "facebook.com",
    "amazon.com",
    "github.com"
};

char *ips[] = {
    "142.250.195.14",
    "142.250.72.14",
    "157.240.241.17",
    "205.251.242.103",
    "140.82.112.4"
};

int main()
{
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientSize;

    char domain[100];
    char response[100];

    // Create UDP socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    // Server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    bind(serverSocket,
         (struct sockaddr *)&serverAddr,
         sizeof(serverAddr));

    printf("DNS Server running on port %d...\n", PORT);

    while (1)
    {
        clientSize = sizeof(clientAddr);

        // Receive domain name
        int n = recvfrom(serverSocket,
                         domain,
                         sizeof(domain) - 1,
                         0,
                         (struct sockaddr *)&clientAddr,
                         &clientSize);

        if (n <= 0)
            continue;

        domain[n] = '\0';

        printf("Query received: %s\n", domain);

        // Simulate timeout
        if (strcmp(domain, "timeout.com") == 0)
        {
            printf("Simulating timeout...\n");
            continue;
        }

        // Invalid format
        if (strchr(domain, '.') == NULL)
        {
            strcpy(response, "INVALID FORMAT");
        }
        else
        {
            int found = 0;

            // Search DNS database
            for (int i = 0; i < 5; i++)
            {
                if (strcmp(domain, domains[i]) == 0)
                {
                    strcpy(response, ips[i]);
                    found = 1;
                    break;
                }
            }

            // Domain not found
            if (!found)
            {
                strcpy(response, "NXDOMAIN");
            }
        }

        // Send response
        sendto(serverSocket,
               response,
               strlen(response) + 1,
               0,
               (struct sockaddr *)&clientAddr,
               clientSize);
    }

    close(serverSocket);

    return 0;
}
