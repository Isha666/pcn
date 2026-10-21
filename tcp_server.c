
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr;
    char rollNo[50];
    char line[200];
    char response[200];

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    bind(serverSocket,
         (struct sockaddr *)&serverAddr,
         sizeof(serverAddr));

    // Listen
    listen(serverSocket, 5);

    printf("Server waiting for client...\n");

    // Accept client
    clientSocket = accept(serverSocket, NULL, NULL);

    printf("Client connected!\n");

    // Set 10-second receive timeout
    struct timeval timer;
    timer.tv_sec = 10;
    timer.tv_usec = 0;

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
               &timer, sizeof(timer));

    // Receive roll number
    int n = recv(clientSocket, rollNo,
                 sizeof(rollNo) - 1, 0);

    if (n <= 0)
    {
        printf("No roll number received.\n");
        close(clientSocket);
        close(serverSocket);
        return 0;
    }

    rollNo[n] = '\0';

    // Remove newline if present
    rollNo[strcspn(rollNo, "\n")] = '\0';

    printf("Roll Number received: %s\n", rollNo);

    // Open CSV file
    FILE *fp = fopen("students.csv", "r");

    if (fp == NULL)
    {
        strcpy(response, "File not found");

        send(clientSocket, response,
             strlen(response) + 1, 0);

        close(clientSocket);
        close(serverSocket);
        return 0;
    }

    // Skip header
    fgets(line, sizeof(line), fp);

    int found = 0;

    // Search CSV
    while (fgets(line, sizeof(line), fp))
    {
        char copy[200];

        strcpy(copy, line);

        // Get roll number from CSV
        char *roll = strtok(copy, ",");

        if (roll != NULL && strcmp(roll, rollNo) == 0)
        {
            // Remove newline
            line[strcspn(line, "\n")] = '\0';

            strcpy(response, line);

            found = 1;
            break;
        }
    }

    fclose(fp);

    // NEW: Simulate delayed response
    printf("Simulating 12-second delay...\n");
    sleep(12);

    // Send result
    if (found)
    {
        send(clientSocket, response,
             strlen(response) + 1, 0);
    }
    else
    {
        strcpy(response, "Student Record Not Found");

        send(clientSocket, response,
             strlen(response) + 1, 0);
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}
