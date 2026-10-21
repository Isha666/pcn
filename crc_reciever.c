
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9200

// CRC division using XOR
void crcDivide(char data[], char gen[], char rem[])
{
    char temp[2048];
    strcpy(temp, data);

    int n = strlen(data);
    int m = strlen(gen);

    for (int i = 0; i <= n - m; i++)
    {
        if (temp[i] == '1')
        {
            for (int j = 0; j < m; j++)
            {
                temp[i + j] = (temp[i + j] == gen[j]) ? '0' : '1';
            }
        }
    }

    strcpy(rem, temp + n - m + 1);
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr;

    char data[2048], rem[10], gen[] = "10011";

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    listen(serverSocket, 5);

    printf("CRC Receiver waiting...\n");

    clientSocket = accept(serverSocket, NULL, NULL);

    int n = recv(clientSocket, data, sizeof(data) - 1, 0);
    if (n <= 0)
    {
        printf("No data received.\n");
        close(clientSocket);
        close(serverSocket);
        return 0;
    }
    data[n] = '\0';

    printf("Received codeword: %s\n", data);

    crcDivide(data, gen, rem);

    printf("Remainder: %s\n", rem);

    if (strcmp(rem, "0000") == 0)
        printf("No error detected!\n");
    else
        printf("Error detected!\n");

    close(clientSocket);
    close(serverSocket);

    return 0;
}
