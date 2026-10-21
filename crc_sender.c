
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9200

// Convert text to binary
void toBinary(char msg[], char binary[])
{
    int k = 0;

    for (int i = 0; msg[i] != '\0'; i++)
    {
        for (int j = 7; j >= 0; j--)
        {
            binary[k++] = ((unsigned char)msg[i] >> j & 1) + '0';
        }
    }

    binary[k] = '\0';
}

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
    int sock;
    struct sockaddr_in addr;

    char msg[100], binary[2048], data[2048];
    char rem[10], gen[] = "10011";

    printf("Enter message: ");
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = '\0';

    toBinary(msg, binary);

    printf("Binary data: %s\n", binary);

    strcpy(data, binary);
    strcat(data, "0000");

    crcDivide(data, gen, rem);

    printf("CRC remainder: %s\n", rem);

    strcpy(data, binary);
    strcat(data, rem);

    printf("Codeword: %s\n", data);

    // Simulate error
    int choice;
    printf("Introduce error? (1=Yes, 0=No): ");
    scanf("%d", &choice);

    if (choice == 1 && strlen(data) > 0)
    {
        data[0] = (data[0] == '0') ? '1' : '0';
        printf("Corrupted codeword: %s\n", data);
    }

    // TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    send(sock, data, strlen(data) + 1, 0);

    printf("Frame sent!\n");

    close(sock);
    return 0;
}
