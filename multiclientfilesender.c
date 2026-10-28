
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;

    char rollNo[50], buffer[200];

    // Create socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect
    if (connect(clientSocket,
                (struct sockaddr *)&serverAddr,
                sizeof(serverAddr)) < 0)
    {
        printf("Connection failed!\n");
        return 1;
    }

    printf("Connected to Server!\n");

    // Enter roll number
    printf("Enter Roll Number: ");
    scanf("%49s", rollNo);

    // Send roll number
    send(clientSocket, rollNo,
         strlen(rollNo) + 1, 0);

    // Receive response
    int n = recv(clientSocket, buffer,
                 sizeof(buffer) - 1, 0);

    if (n > 0)
    {
        buffer[n] = '\0';

        // Timestamp
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        printf("\n[%02d:%02d:%02d] Server Response:\n",
               t->tm_hour, t->tm_min, t->tm_sec);

        printf("%s\n", buffer);
    }
    else
    {
        printf("No response received.\n");
    }

    close(clientSocket);
    return 0;
}
