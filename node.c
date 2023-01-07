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
#define MAXNODES 6

struct parameters
{
    int socketfd_param;
    uint16_t *known_ports_param;
    size_t *known_ports_count_param;
};

// update_known_ports(uint16_t *known_ports, size_t *known_ports_count)
// {

// }

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
    struct parameters *params = (struct parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_ports = params->known_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    // printf("socketfd: %d and known_ports: %d\n", socketfd, known_ports[0]);

    while (1)
    {
        char message[MAXLINE];

        struct sockaddr_in cliaddr;
        int len = sizeof(cliaddr);

        int n = recvfrom(socketfd, (char *)message, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        message[n] = '\0';

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, INET_ADDRSTRLEN);

        // if (strcmp(message, "heartbeat") == 0)
        // {
        //     update_known_ports(known_ports, known_ports_count);
        // }

        uint16_t sender_port = ntohs(cliaddr.sin_port);

        printf("sender address: %s:%d and he send this: %s\n", ip, sender_port, message);


        // TODO: save port as known node (clear array and fill them with ports from heartbeat)
    }
    return NULL;
}

void send_message(int socketfd, char *message, uint16_t *known_ports, size_t known_ports_count)
{
    for (size_t i = 0; i < known_ports_count; i++)
    {
        printf("%d\n", known_ports[i]);
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(known_ports[i]);

        sendto(socketfd, message, strlen(message), MSG_DONTWAIT, (const struct sockaddr *)&servaddr, sizeof(servaddr));

        close(socketfd);
    }
}

// Driver code
int main(const int argc, const char *argv[])
{
    int socketfd;
    struct parameters params;
    size_t *known_ports_count = (size_t *)calloc(1, sizeof(size_t));

    int port = atoi(argv[1]);
    int port_to_connect = atoi(argv[2]);

    uint16_t *known_ports = (uint16_t *)calloc(MAXNODES * sizeof(uint16_t));

    if (port_to_connect != 0)
    {
        // known_ports[*known_ports_count] = port_to_connect;
        // known_ports_count++;
        known_ports[0] = 8080;
        known_ports[1] = 8081;
        *known_ports_count = 2;
    }

    socketfd = create_socket_and_bind(port);

    params.socketfd_param = socketfd;
    params.known_ports_param = known_ports;
    params.known_ports_count_param = known_ports_count;

    pthread_t thread_id;
    // todo: heartbeat
    pthread_create(&thread_id, NULL, listen_messages, &params);

    printf("Type help to get some help\n");

    while (1)
    {
        char input[20];
        scanf("%s", input);
        if (strcmp(input, "send") == 0)
        {
            char message[50];
            printf("\tMessage: ");
            scanf("%s", message);
            send_message(socketfd, message, known_ports, *known_ports_count);
        }
        else if (strcmp(input, "help") == 0)
        {
            printf("\tShowing help:\n");
            printf("\t\texit\t end program\n");
            printf("\t\tsend\t send message to all known nodes\n");
        }
        else if (strcmp(input, "exit") == 0)
        {
            printf("\tExiting...\n");
            free(known_ports);
            pthread_cancel(thread_id);
            return 0;
        }
        else
        {
            printf("\tUnknown command!\n");
        }
    }

    pthread_join(thread_id, NULL);

    return 0;
}