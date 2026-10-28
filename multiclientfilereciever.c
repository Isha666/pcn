
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

void *handleClient(void *arg)
{
    int clientSocket = *(int *)arg;
    free(arg);

    char rollNo[50], line[200], response[200];

    // Receive roll number
    int n = recv(clientSocket, rollNo,
                 sizeof(rollNo) - 1, 0);

    if (n <= 0)
    {
        close(clientSocket);
        return NULL;
    }

    rollNo[n] = '\0';

    printf("Roll Number received: %s\n", rollNo);

    // Open CSV file
    FILE *fp = fopen("students.csv", "r");

    if (fp == NULL)
    {
        strcpy(response, "File not found");
    }
    else
    {
        // Skip header
        fgets(line, sizeof(line), fp);

        int found = 0;

        // Search CSV
        while (fgets(line, sizeof(line), fp))
        {
            char copy[200];
            strcpy(copy, line);

            char *roll = strtok(copy, ",");

            if (roll != NULL && strcmp(roll, rollNo) == 0)
            {
                line[strcspn(line, "\n")] = '\0';
                strcpy(response, line);
                found = 1;
                break;
            }
        }

        fclose(fp);

        if (!found)
            strcpy(response, "Student Record Not Found");
    }

    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    printf("[%02d:%02d:%02d] Response to %s: %s\n",
           t->tm_hour, t->tm_min, t->tm_sec,
           rollNo, response);

    // Send response
    send(clientSocket, response, strlen(response) + 1, 0);

    close(clientSocket);
    return NULL;
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr;
    pthread_t thread;

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

    printf("Server waiting for clients...\n");

    while (1)
    {
        // Accept client
        clientSocket = accept(serverSocket, NULL, NULL);

        if (clientSocket < 0)
            continue;

        printf("Client connected!\n");

        // Create separate thread
        int *p = malloc(sizeof(int));
        *p = clientSocket;

        pthread_create(&thread, NULL, handleClient, p);
        pthread_detach(thread);
    }

    close(serverSocket);
    return 0;
}
