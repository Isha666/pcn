
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9200

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
                temp[i + j] = (temp[i + j] == gen[j]) ? '0' : '1';
        }
    }

    strcpy(rem, temp + n - m + 1);
}

void *handleClient(void *arg)
{
    int clientSocket = *(int *)arg;
    free(arg);

    char packet[2100], data[2048], gen[32], rem[32];

    int n = recv(clientSocket, packet, sizeof(packet) - 1, 0);

    if (n <= 0)
    {
        close(clientSocket);
        return NULL;
    }

    packet[n] = '\0';

    // Extract generator and codeword
    char *p = strchr(packet, '|');

    if (p == NULL)
    {
        close(clientSocket);
        return NULL;
    }

    *p = '\0';
    strcpy(gen, packet);
    strcpy(data, p + 1);

    clock_t start = clock();

    crcDivide(data, gen, rem);

    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\nClient %d\n", clientSocket);
    printf("Generator: %s\n", gen);
    printf("Codeword size: %zu bits\n", strlen(data));
    printf("Remainder: %s\n", rem);
    printf("CRC checking time: %.9f seconds\n", elapsed);

    int error = 0;

    for (int i = 0; rem[i] != '\0'; i++)
    {
        if (rem[i] != '0')
            error = 1;
    }

    if (error == 0)
        printf("No error detected!\n");
    else
        printf("Error detected!\n");

    close(clientSocket);
    return NULL;
}

int main()
{
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr;
    pthread_t thread;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr *)&serverAddr,
         sizeof(serverAddr));

    listen(serverSocket, 5);

    printf("CRC Receiver waiting...\n");

    while (1)
    {
        clientSocket = accept(serverSocket, NULL, NULL);

        if (clientSocket < 0)
            continue;

        int *p = malloc(sizeof(int));
        *p = clientSocket;

        pthread_create(&thread, NULL, handleClient, p);
        pthread_detach(thread);
    }

    close(serverSocket);
    return 0;
}
/*how to run
gcc receiver.c -o receiver -pthread
./receiver

gcc sender.c -o sender
./sender

./sender*/