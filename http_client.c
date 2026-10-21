
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8000

int main()
{
    int sock, n;
    struct sockaddr_in server;
    char request[] = "GET /index.html HTTP/1.0\r\n\r\n";
    char buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Connection failed\n");
        return 1;
    }

    send(sock, request, strlen(request), 0);

    FILE *fp = fopen("downloaded.html", "wb");

    int header = 0;

    while ((n = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        if (!header)
        {
            for (int i = 0; i < n - 3; i++)
            {
                if (memcmp(buffer + i, "\r\n\r\n", 4) == 0)
                {
                    fwrite(buffer + i + 4, 1, n - i - 4, fp);
                    header = 1;
                    break;
                }
            }
        }
        else
        {
            fwrite(buffer, 1, n, fp);
        }
    }

    fclose(fp);
    close(sock);

    printf("Webpage downloaded as downloaded.html\n");

    return 0;
}
