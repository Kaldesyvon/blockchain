#include "node.h"

static int port;

int main(const int argc, const char *argv[]) // todo: add alive nodes list that newly created thread will count aliveness of node and if trehsold is exceded, than remove from known nodes
{
    port = atoi(argv[1]);
    int port_to_connect = atoi(argv[2]);

    int socketfd;
    Parameters params;

    uint16_t *known_ports = (uint16_t *)calloc(MAXNODES + 1, sizeof(uint16_t)); // ensure that node has room for its own port to send over heartbeat
    time_t *alive_ports = (time_t *)calloc(MAXNODES, sizeof(time_t));
    size_t *known_ports_count = (size_t *)calloc(1, sizeof(size_t));

    if (port_to_connect != 0)
    {
        struct timeval time;

        gettimeofday(&time, NULL);
        alive_ports[*known_ports_count] = time.tv_sec;

        known_ports[*known_ports_count] = port_to_connect;
        *known_ports_count += 1;
    }

    socketfd = create_socket_and_bind(port);

    params.socketfd_param = socketfd;
    params.known_ports_param = known_ports;
    params.alive_ports_param = alive_ports;
    params.known_ports_count_param = known_ports_count;

    pthread_t listen_thread_id, listen_heartbeat_thread_id, send_heartbeat_thread_id;

    pthread_create(&listen_thread_id, NULL, listen_messages, &params);
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
        else if (strcmp(input, "print") == 0)
        {
            if (*known_ports_count == 0)
            {
                printf("\tno node is known");
            }
            print_known_nodes(known_ports, *known_ports_count);
        }
        else if (strcmp(input, "help") == 0)
        {
            printf("\tshowing help:\n");
            printf("\t\tprint\t print known nodes\n");
            printf("\t\texit\t end program\n");
            printf("\t\tsend\t send message to all known nodes\n");
        }
        else if (strcmp(input, "exit") == 0)
        {
            printf("\texiting...\n");
            free(known_ports);
            pthread_cancel(send_heartbeat_thread_id);
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

    return 0;
}

void *listen_messages(void *arg)
{
    Parameters *params = (Parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_ports = params->known_ports_param;
    time_t *alive_ports = params->alive_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    while (1)
    {
        Packet packet;

        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        int n = recvfrom(socketfd, &packet, sizeof(Packet), MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, INET_ADDRSTRLEN);

        uint16_t sender_port = ntohs(cliaddr.sin_port);

        if (packet.type == MSG_TYPE_ORDINARY)
        {
            printf("sender address: %s:%d and he send this: %s\n", ip, sender_port, packet.data.message);
        }
        else if (packet.type == MSG_TYPE_HEARTBEAT)
        {
            printf("got heartbeat\n");
            if (*known_ports_count == MAXNODES)
            {
                printf("this node has maximum count of known nodes!\n");
                continue;
            }

            uint16_t *received_ports = (uint16_t *)calloc(MAXNODES, sizeof(uint16_t));

            memcpy(received_ports, packet.data.ports, sizeof(packet.data.ports));

            merge_known_ports(known_ports, *known_ports_count, received_ports, get_length(received_ports));
            *known_ports_count = get_length(known_ports);

            free(received_ports);
        }
        else
        {
            printf("cannot distinguish message type");
        }
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

        Packet packet;

        packet.type = MSG_TYPE_ORDINARY;
        memcpy(packet.data.message, message, strlen(message) + 1); // ensures terminator

        sendto(socketfd, &packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
}

void *send_heartbeat(void *arg)
{
    Parameters *params = (Parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_ports = params->known_ports_param;
    time_t *alive_ports = params->alive_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    while (1)
    {
        sleep(HEARTBEAT_TIMER);

        for (size_t i = 0; i < *known_ports_count; i++)
        {
            struct sockaddr_in servaddr;

            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = INADDR_ANY;
            servaddr.sin_port = htons(known_ports[i]);

            Packet packet;

            packet.type = MSG_TYPE_HEARTBEAT;
            memcpy(packet.data.ports, known_ports, sizeof(known_ports));
            append(packet.data.ports, port);

            sendto(socketfd, &packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

            // printf("heartbeat sent to %d\n", known_ports[i]);
        }

        reduce_ports(known_ports, alive_ports, *known_ports_count);
    }

    return NULL;
}

void reduce_ports(uint16_t *known_ports, time_t *alive_ports, size_t known_ports_count)
{
    for (size_t i = 0; i < known_ports_count; i++)
    {
        struct timeval now;
        gettimeofday(&now, NULL);

        if (now.tv_sec - alive_ports[i] < HEARTBEAT_TIMER * 3)
        {
            
        }
    }
}

int create_socket_and_bind(int port)
{
    // todo: sockopt
    int socketfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socketfd < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in servaddr;

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

void print_known_nodes(uint16_t *known_ports, size_t known_ports_count)
{
    for (size_t i = 0; i < known_ports_count; i++)
    {
        printf("\tknown node: %d\n", known_ports[i]);
    }
}

void merge_known_ports(uint16_t *known_ports, size_t known_ports_count, uint16_t *received_ports, size_t received_ports_count)
{
    for (size_t i = 0; i < received_ports_count; i++)
    {
        if (received_ports[i] == port)
            continue;
        bool is_present = false;
        for (size_t j = 0; j < known_ports_count; j++)
        {

            if (known_ports[j] == received_ports[i])
            {
                is_present = true;
                break;
            }
        }
        if (!is_present)
        {
            append(known_ports, received_ports[i]);
        }
    }
}

void append(uint16_t *array, uint16_t port)
{
    size_t size = get_length(array);
    if (size < MAXNODES)
    {
        array[size] = port;
    }
    else
    {
        printf("cannot append to array with maximum length");
    }
}

size_t get_length(uint16_t *array)
{
    size_t size = 0;
    while (array[size] != 0 && size < MAXNODES)
    {
        size++;
    }
    return size;
}