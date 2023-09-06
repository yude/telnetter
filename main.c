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
#include <time.h>
#include <signal.h>
#include <assert.h>

void *handle_connection(void *threadArg);
int start_server(int port);
char *replace_string(char* s, const char* before, const char* after);
char *get_datetime();
void remove_char(char *str, char target);

struct ThreadArgs
{
    int soc;
    int visitor;
};

int main(int argc, char const *argv[])
{
    signal(SIGPIPE, SIG_IGN);

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

char *get_datetime()
{
    struct tm fmt_time;
    char *fmt_week[] = {"日", "月", "火", "水", "木", "金", "土"};
    time_t t = time(NULL);
    localtime_r(&t, &fmt_time);
    static char time_buf[1024];

    snprintf(
        time_buf, sizeof(time_buf),
        "現在の時刻は %d/%d/%d (%s) %d:%02d:%02d (JST) です。\n",
        fmt_time.tm_year + 1900, fmt_time.tm_mon + 1, fmt_time.tm_mday,
        fmt_week[fmt_time.tm_wday], fmt_time.tm_hour, fmt_time.tm_min, fmt_time.tm_sec
    );

    return time_buf;
}

char *load_message(char *file_name)
{
    FILE *fp;
    char buf[256];
    static char content[2048];

    memset(buf, '\0', sizeof(buf));
    memset(content, '\0', sizeof(content) - 1);

    if (content[0] != '\0')
    {
        return content;
    }    

    fp = fopen(file_name, "r");
    if (fp == NULL)
    {
        fprintf(stdout, "Error: Failed to open %s.\n", file_name);
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
    int idx = 0;
    char buf[512];

    pthread_detach(pthread_self());

    soc = ((struct ThreadArgs *)thread_args)->soc;
    visitor = ((struct ThreadArgs *)thread_args)->visitor;

    free(thread_args);

    char *content = NULL;
    char *message_file_name = "content/message.txt";
    content = load_message(message_file_name);

    char *time_buf = NULL;
    time_buf = get_datetime();

    char dynamic_message[2048];
    snprintf(
        dynamic_message,
        sizeof(dynamic_message),
        "あなたはサーバーを再起動してから %d 回目のお客様です。\n",
        visitor
    );
    strcat(dynamic_message, time_buf);

    const char dynamic_template[] = "{dynamic}";

    replace_string(
        content,
        dynamic_template,
        dynamic_message
    );

    while (content[idx] != '\0')
    {
        write(soc, &content[idx], 1);
        idx++;
        usleep(9000);
    }

    char command_buf[1024];

    while (1)
    {
        COMMAND_BEGIN:

        memset(command_buf, '\0', sizeof(command_buf) - 1);
        
        const char command_reference_message[] =
        "なにか命令を入力してください。help でヘルプを表示します。\n[telnet.yude.jp]> ";
        
        idx = 0;
        while (command_reference_message[idx] != '\0')
        {
            write(soc, &command_reference_message[idx], 1);
            idx++;
            usleep(9000);
        }
        
        read(soc, &command_buf, sizeof(command_buf));

        char *text_content = NULL;
        char text_file_name[2048];

        remove_char(command_buf, '\n');
        remove_char(command_buf, '\r');

        snprintf(text_file_name, sizeof(text_file_name), "content/%s.txt", &command_buf);

        text_content = load_message(text_file_name);

        if (text_content != NULL) {
            idx = 0;

            while (text_content[idx] != '\0')
            {
                write(soc, &text_content[idx], 1);
                idx++;
                usleep(9000);
            }

            goto COMMAND_BEGIN;
        }

        // Command: `now`
        char now_command[] = "now";
        if (
          strncmp(now_command, command_buf, strlen(now_command)) == 0
        )
        {
            char *now_content = NULL;
            now_content = get_datetime();

            idx = 0;

            while (now_content[idx] != '\0')
            {
                write(soc, &now_content[idx], 1);
                idx++;
                usleep(9000);
            }

            goto COMMAND_BEGIN;
        }
        
        const char unknown_command_message[]
        = "無効な命令です。\n";

        idx = 0;
        while (unknown_command_message[idx] != '\0')
        {
            write(soc, &unknown_command_message[idx], 1);
            idx++;
            usleep(9000);
        }
    }

    close(soc);
}

char *replace_string(char* s, const char* before, const char* after)
{
    assert(s != NULL);
    assert(before != NULL);
    assert(after != NULL);

    const size_t before_len = strlen(before);
    if (before_len == 0) {
        return s;
    }

    const size_t after_len = strlen(after);
    char* p = s;

    while (1)
    {
        p = strstr(p, before);
        if (p == NULL)
        {
            break;
        }

        const char* p2 = p + before_len;

        memmove(p + after_len, p2, strlen(p2) + 1);

        memcpy(p, after, after_len);

        p += after_len;
    }

    return s;
}

void remove_char(char *str, char target) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != target) dst++;
    }
    *dst = '\0';
}