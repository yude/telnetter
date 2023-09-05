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
int start_server(int port);

struct ThreadArgs
{
    int soc;
    int visitor;
};

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

    fprintf(stdout, "Starting server on port %d.\n", port);
    
    start_server(port);

    return 0;
}

char *read_message()
{
    FILE *fp;
    char buf[256];
    char file_name[] = "message.txt";
    static char content[1024];

    if (content[0] != '\0')
    {
        return content;
    }    

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Failed to open %s.\n", file_name);
        return NULL;
    }
    else
    {
        fprintf(stdout, "Info: Loading message from %s.\n", file_name);
    }

    int chr;
    while ((chr = fgetc(fp)) != EOF)
    {
        sprintf(buf, "%c", chr);
        strcat(content, buf);
    }

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

    char host_buf[NI_MAXHOST];
    char service_buf[NI_MAXSERV];

    int visitor = 1;

    if (
        (
            soc_listen = socket(AF_INET, SOCK_STREAM, 0)
        ) < 0
    )
    {
        fprintf(stderr, "Error: Failed to listen port.\n");
        exit(-1);
    }

    if (
        bind(
            soc_listen, (struct sockaddr *)&addr, sizeof(addr)
        ) == -1
    )
    {
        fprintf(stderr, "Error: Failed to listen port.\n");
        exit(-1);
    }

    if (
        listen(soc_listen, 1) != 0
    )
    {
        fprintf(stderr, "Error: Failed to open port.\n");
        exit(-1);
    }

    while (1)
    {
        struct sockaddr addr_io;
        socklen_t addr_io_len;
        
        addr_io_len = sizeof(addr_io);
        soc_io = accept(soc_listen, &addr_io, &addr_io_len);

        int res;
        res = getnameinfo(
                &addr_io,
                addr_io_len,
                host_buf,
                sizeof(host_buf),
                service_buf,
                sizeof(service_buf),
                NI_NUMERICSERV
            );
        
        // Retrieve rich error-code from getnameinfo()
        // printf("%s\n", gai_strerror(res));

        if (
            res == 0    
        )
        {
            fprintf(stdout, "Info: Accepting remote connection from %s:%s\n", host_buf, service_buf);
        }
        else
        {
            fprintf(stdout, "Info: Accepting from remote connection (Unknown origin).\n");
        }

        if ((thread_args = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs))) == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory.\n");
            exit(-1);
        }

        thread_args->soc = soc_io;
        thread_args->visitor = visitor++;

        if (
            pthread_create(&thread_id, NULL, handle_connection, (void *)thread_args) != 0
        )
        {
            fprintf(stderr, "Error: Failed to create thread.\n");
            exit(-1);
        }

    }

    return 0;
}

void *handle_connection(void *thread_args)
{
    int soc, num, visitor;
    char buf[512];

    pthread_detach(pthread_self());

    soc = ((struct ThreadArgs *)thread_args)->soc;
    visitor = ((struct ThreadArgs *)thread_args)->visitor;

    free(thread_args);

    char *content = NULL;
    content = read_message();

    char message[2048];
    snprintf(message, sizeof(message), "あなたはサーバーを再起動してから %d 回目のお客様です！！！！！！！！\n", visitor);
    strcat(message, content);
    
    write(soc, message, 2048);

    close(soc);
}
