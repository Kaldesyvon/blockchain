#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAXLINE 1024

static int port = 8081;

int create_socket_and_bind(int port)
{
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(socketfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("binding success!\n");
    return socketfd;
}

void *listen_messages(void *arg)
{
    int socketfd = *(int *)arg;
    while (1)
    {
        int len, n;
        int success_response = htonl(0);

        char buffer[MAXLINE];

        struct sockaddr_in cliaddr;

        len = sizeof(cliaddr);

        n = recvfrom(socketfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, INET_ADDRSTRLEN);

        uint16_t sender_port = ntohs(cliaddr.sin_port);

        printf("sender address: %s:%d\nhe send this: %s\n", ip, sender_port, buffer);

        // TODO: save port as known node
    }
    return NULL;
}

void send_message(int socketfd, char *message)
{
    int n, len;

    struct sockaddr_in servaddr;

    char buffer[MAXLINE];

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8080);

    sendto(socketfd, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    printf("message sent.\n");

    n = recvfrom(socketfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&servaddr, &len);
    buffer[n] = '\0';
    printf("Server : %s\n", buffer);

    close(socketfd);
}

// Driver code
int main(const int argc, const char *argv[])
{
    // port = atoi(argv[1]);
    // int port_to_connect = atoi(argv[2]);

    int socketfd;
    char buffer[MAXLINE];

    socketfd = create_socket_and_bind(port);

    pthread_t thread_id;
    printf("Before Thread\n");
    pthread_create(&thread_id, NULL, listen_messages, &socketfd);

    send_message(socketfd, "hello there!");

    pthread_join(thread_id, NULL);

    printf("After Thread\n");

    return 0;
}