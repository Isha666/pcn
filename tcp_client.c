
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>

#define PORT 8080

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;

    char rollNo[50];
    char buffer[200];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect
    connect(clientSocket,
            (struct sockaddr *)&serverAddr,
            sizeof(serverAddr));

    printf("Connected to Server\n");

    // Set 10-second receive timeout
    struct timeval timer;
    timer.tv_sec = 10;
    timer.tv_usec = 0;

    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO,
               &timer, sizeof(timer));

    // Enter roll number
    printf("Enter Roll Number: ");
    scanf("%49s", rollNo);

    // Send roll number
    send(clientSocket,
         rollNo,
         strlen(rollNo) + 1,
         0);

    // Receive response
    int n = recv(clientSocket,
                 buffer,
                 sizeof(buffer) - 1,
                 0);

    if (n < 0)
    {
        printf("Packet not received within 10 seconds! Timeout occurred.\n");
    }
    else if (n == 0)
    {
        printf("Server closed connection.\n");
    }
    else
    {
        buffer[n] = '\0';

        if (strcmp(buffer, "Student Record Not Found") == 0 ||
            strcmp(buffer, "File not found") == 0)
        {
            printf("%s\n", buffer);
        }
        else
        {
            printf("\nStudent Record:\n");
            printf("%s\n", buffer);
        }
    }

    close(clientSocket);

    return 0;
}
