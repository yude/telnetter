#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>

void *handle_connection(void *threadArg);

struct ThreadArgs
{
    int soc;
};

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
    struct sockaddr_in addr;
    
    int soc_listen;
    int soc_io;

    pthread_t thread_id;
    struct ThreadArgs *thread_args;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (
        (
            soc_listen = socket(AF_INET, SOCK_STREAM, 0)
        ) < 0
    )
    {
        perror("Error: Failed to listen port.\n");
        exit(1);
    }

    if (
        bind(
            soc_listen, (struct sockaddr *)&addr, sizeof(addr)
        ) == -1
    )
    {
        perror("Error: Failed to listen port.\n");
        exit(1);
    }

    listen(soc_listen, 1);

    while (1)
    {
        soc_io = accept(soc_listen, NULL, NULL);

        if ((thread_args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs))) == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory.\n");
            exit(1);
        }

        thread_args->soc = soc_io;

        if (
            pthread_create(&thread_id, NULL, handle_connection, (void *)thread_args) != 0
        ) {
            fprintf(stderr, "Error: Failed to create thread.\n");
            exit(1);
        }

    }

    return 0;
}

void *handle_connection(void *thread_args)
{
    int soc, num;
    char buf[512];

    pthread_detach(pthread_self());

    soc = ((struct ThreadArgs *)thread_args)->soc;

    free(thread_args);

    char *message = NULL;
    message = read_message();
    write(soc, message, 1024);

    close(soc);
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
    fprintf(stdout, "Starting server on port %d.\n", port);
    
    start_server(port);

    return 0;
}
