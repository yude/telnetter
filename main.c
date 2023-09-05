#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

char *read_message()
{
    FILE *fp;
    char buf[256];
    char file_name[] = "message.txt";
    static char content[1024] = "";

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Failed to open %s, does this exist?\n", file_name);
        return NULL;
    }
    else
    {
        printf("Info: Loading message from %s.\n", file_name);
    }

    int chr;
    while ((chr = fgetc(fp)) != EOF)
    {
        sprintf(buf, "%c", chr);
        strcat(content, buf);
    }
    printf("Info: Showing the content below\n%s\n", content);

    fclose(fp);

    return content;
}

int start_server(int port)
{
    int sock_w, sock_r;
    struct sockaddr_in addr, client;
    socklen_t len;
    int ret;

    char *message = NULL;
    message = read_message();

    sock_r = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_r < 0)
    {
        fprintf(stderr, "Error: Failed to create socket.\n");
        return 1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sock_r, (struct sockaddr *)&addr, sizeof(addr));

    if (ret < 0)
    {
        fprintf(stderr, "Error: Failed to bind to socket.\n");
        return 1;
    }
    listen(sock_r, 5);
    while (1)
    {
        len = sizeof(client);
        sock_w = accept(sock_r, (struct sockaddr *)&client, &len);

        write(sock_w, message, 1024);
    }

    close(sock_w);
    close(sock_r);

    return 0;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        printf("Usage:\n");
        printf("  %s <port>\n", argv[0]);

        return 1;
    }

    if (!isdigit(*argv[1]))
    {
        printf("Usage:\n");
        printf("  %s <port>\n", argv[0]);

        return 1;
    }

    const int port = atoi(argv[1]);

    if (!(port >= 0 && port <= 65535))
    {
        printf("Usage:\n");
        printf("  %s <port>\n", argv[0]);
        printf("  Hint: Port number should be greater than or equal to 0, and less than or equal to 65535.\n");

        return 1;
    }

    printf("Starting server on port %d.\n", port);
    start_server(port);

    return 0;
}
