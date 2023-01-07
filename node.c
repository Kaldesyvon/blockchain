#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define MAXLINE 1024
#define MAXNODES 6
#define MSG_TYPE_HEARTBEAT 0
#define MSG_TYPE_ORDINARY  1

struct message {
    uint8_t type;  // Message type (heartbeat or ordinary)
    uint16_t data; // Data field (for ordinary messages)
};

struct parameters
{
    int socketfd_param;
    uint16_t *known_ports_param;
    size_t *known_ports_count_param;
};

void print_known_nodes(uint16_t *known_ports, size_t known_ports_count)
{
    for (size_t i = 0; i < known_ports_count; i++)
    {
        printf("known node: %d\n", known_ports[i]);
    }
}

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

    return socketfd;
}

void *listen_messages(void *arg)
{
    struct parameters *params = (struct parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_ports = params->known_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    while (1)
    {
        char message[MAXLINE];

        struct sockaddr_in cliaddr;
        int len = sizeof(cliaddr);

        int n = recvfrom(socketfd, (char *)message, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        message[n] = '\0';

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, INET_ADDRSTRLEN);

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
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(known_ports[i]);

        sendto(socketfd, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
}

void *send_heartbeat(void *arg)
{
    struct parameters *params = (struct parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_ports = params->known_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    char *message = "heartbeat"; // todo: send known hosts

    while (1)
    {
        sleep(10);

        for (size_t i = 0; i < *known_ports_count; i++)
        {
            struct sockaddr_in servaddr;

            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = INADDR_ANY;
            servaddr.sin_port = htons(known_ports[i]);

            int n, len;

            sendto(socketfd, message, strlen(message), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

            printf("heartbeat sent to %d\n", known_ports[i]);

            print_known_nodes(known_ports, *known_ports_count);
        }
    }

    return NULL;
}

void *listen_heartbeat(void *arg)
{
    struct parameters *params = (struct parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_ports = params->known_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    return NULL;
}

// Driver code
int main(const int argc, const char *argv[]) // todo: known hosts should not be allocated, nodes needs to send it todo: create struct of message, that will determine if thats heartbeat or message
{
    int port = atoi(argv[1]);
    int port_to_connect = atoi(argv[2]);

    int socketfd;
    struct parameters params;

    uint16_t *known_ports = (uint16_t *)calloc(MAXNODES, sizeof(uint16_t));
    size_t *known_ports_count = (size_t *)calloc(1, sizeof(size_t));

    if (port_to_connect != 0)
    {
        known_ports[*known_ports_count] = port_to_connect;
        *known_ports_count += 1;
        print_known_nodes(known_ports, *known_ports_count);
    }

    socketfd = create_socket_and_bind(port);

    params.socketfd_param = socketfd;
    params.known_ports_param = known_ports;
    params.known_ports_count_param = known_ports_count;

    pthread_t listen_thread_id, listen_heartbeat_thread_id, send_heartbeat_thread_id;

    pthread_create(&listen_thread_id, NULL, listen_messages, &params);
    pthread_create(&listen_heartbeat_thread_id, NULL, listen_heartbeat, &params);
    pthread_create(&send_heartbeat_thread_id, NULL, send_heartbeat, &params);

    printf("Type help to get some help\n");

    while (1)
    {
        char input[20];
        scanf("%s", input);
        if (strcmp(input, "send") == 0)
        {
            char message[50];
            printf("\tmessage: ");
            scanf("%s", message);
            send_message(socketfd, message, known_ports, *known_ports_count);
        }
        else if (strcmp(input, "help") == 0)
        {
            printf("\tshowing help:\n");
            printf("\t\texit\t end program\n");
            printf("\t\tsend\t send message to all known nodes\n");
        }
        else if (strcmp(input, "exit") == 0)
        {
            printf("\texiting...\n");
            free(known_ports);
            pthread_cancel(send_heartbeat_thread_id);
            pthread_cancel(listen_heartbeat_thread_id);
            pthread_cancel(listen_thread_id);
            break;
        }
        else
        {
            printf("\tunknown command!\n");
        }
    }

    pthread_join(send_heartbeat_thread_id, NULL);
    pthread_join(listen_thread_id, NULL);
    pthread_join(listen_heartbeat_thread_id, NULL);

    return 0;
}