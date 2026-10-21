
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main()
{
    int serverSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientSize;

    char rollNo[50];
    char line[200];
    char response[200];

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

    printf("UDP Server waiting for client...\n");

    clientSize = sizeof(clientAddr);

    // Receive roll number
    int n = recvfrom(serverSocket,
                     rollNo,
                     sizeof(rollNo) - 1,
                     0,
                     (struct sockaddr *)&clientAddr,
                     &clientSize);

    if (n <= 0)
    {
        printf("No roll number received.\n");
        close(serverSocket);
        return 0;
    }

    rollNo[n] = '\0';

    printf("Roll Number received: %s\n", rollNo);

    // Open CSV file
    FILE *fp = fopen("students.csv", "r");

    if (fp == NULL)
    {
        strcpy(response, "File not found");

        sendto(serverSocket,
               response,
               strlen(response) + 1,
               0,
               (struct sockaddr *)&clientAddr,
               clientSize);

        close(serverSocket);
        return 0;
    }

    // Skip CSV header
    fgets(line, sizeof(line), fp);

    int found = 0;

    // Search CSV
    while (fgets(line, sizeof(line), fp))
    {
        char copy[200];

        strcpy(copy, line);

        // Extract roll number
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

    // NEW: Simulate delayed response
    printf("Simulating 12-second delay...\n");
    sleep(12);

    // Send result
    if (found)
    {
        sendto(serverSocket,
               response,
               strlen(response) + 1,
               0,
               (struct sockaddr *)&clientAddr,
               clientSize);
    }
    else
    {
        strcpy(response, "Student Record Not Found");

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
