
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9200

void toBinary(char msg[], char binary[])
{
    int k = 0;

    for (int i = 0; msg[i] != '\0'; i++)
    {
        for (int j = 7; j >= 0; j--)
            binary[k++] = ((unsigned char)msg[i] >> j & 1) + '0';
    }

    binary[k] = '\0';
}

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

int main()
{
    int sock, choice;
    struct sockaddr_in addr;

    char msg[100], binary[2048], data[2048];
    char rem[32], gen[32], packet[2100];

    printf("Enter message: ");
    fgets(msg, sizeof(msg), stdin);
    msg[strcspn(msg, "\n")] = '\0';

    printf("Enter generator polynomial: ");
    scanf("%31s", gen);

    int m = strlen(gen);

    if (m < 2 || gen[0] != '1' || gen[m-1] != '1')
    {
        printf("Invalid generator!\n");
        return 1;
    }

    for (int i = 0; i < m; i++)
    {
        if (gen[i] != '0' && gen[i] != '1')
        {
            printf("Generator must contain only 0 and 1.\n");
            return 1;
        }
    }

    toBinary(msg, binary);

    strcpy(data, binary);

    // Append m-1 zeros
    for (int i = 0; i < m - 1; i++)
        strcat(data, "0");

    // Start timer
    clock_t start = clock();

    crcDivide(data, gen, rem);

    // Stop timer
    clock_t end = clock();

    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("Data size: %zu bits\n", strlen(binary));
    printf("CRC size: %d bits\n", m - 1);
    printf("CRC remainder: %s\n", rem);
    printf("CRC calculation time: %.9f seconds\n", elapsed);

    strcpy(data, binary);
    strcat(data, rem);

    printf("Codeword: %s\n", data);

    printf("Introduce error? (1=Yes, 0=No): ");
    scanf("%d", &choice);

    if (choice == 1 && strlen(data) > 0)
        data[0] = (data[0] == '0') ? '1' : '0';

    // Send generator and codeword together
    snprintf(packet, sizeof(packet), "%s|%s", gen, data);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("Connection failed!\n");
        return 1;
    }

    send(sock, packet, strlen(packet) + 1, 0);

    printf("Frame sent!\n");

    close(sock);
    return 0;
}
/*how to run
gcc receiver.c -o receiver -pthread
./receiver

gcc sender.c -o sender
./sender

./sender*/