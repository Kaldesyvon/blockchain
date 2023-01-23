#include "node.h"

static bool run_threads = true;
static uint16_t port;

int main(const int argc, const char *argv[]) // todo: add alive nodes list that newly created thread will count aliveness of node and if trehsold is exceded, than remove from known nodes
{
    port = atoi(argv[1]);
    int port_to_connect = atoi(argv[2]);

    int socketfd;
    Parameters params;

    known_nodes = (uint16_t *)malloc(MAXNODES * sizeof(uint16_t));
    memset(known_nodes, 0, MAXNODES + 1 * sizeof(uint16_t));

    known_nodes_alive_time = (struct timeval *)malloc(MAXNODES * sizeof(struct timeval));

    known_nodes_count = (size_t *)malloc(sizeof(size_t));
    *known_nodes_count = 0;

    if (port_to_connect != 0)
    {
        known_nodes[*known_nodes_count] = port_to_connect;
        *known_nodes_count += 1;
    }

    socketfd = create_socket_and_bind(port);

    params.socketfd_param = socketfd;
    params.known_nodes_param = known_nodes;
    params.known_nodes_count_param = known_nodes_count;

    pthread_t listen_thread_id, listen_heartbeat_thread_id, send_heartbeat_thread_id;

    pthread_create(&listen_thread_id, NULL, listen_messages, &params);
    pthread_create(&send_heartbeat_thread_id, NULL, send_heartbeat, &params);

    printf("Type help to get some help\n");

    while (run_threads)
    {
        char input[20];
        scanf("%s", input);
        if (strcmp(input, "send") == 0)
        {
            char message[1024];
            printf("\tmessage: ");
            scanf("%s", message);
            send_message(socketfd, message, known_nodes, *known_nodes_count);
        }
        else if (strcmp(input, "print") == 0)
        {
            if (*known_nodes_count == 0)
            {
                printf("\tno node is known");
            }
            print_known_nodes(known_nodes, *known_nodes_count);
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
            run_threads = false;
            break;
        }
        else
        {
            printf("\tunknown command!\n");
        }
    }

    pthread_join(send_heartbeat_thread_id, NULL);
    pthread_join(listen_thread_id, NULL);

    free(known_nodes);
    free(known_nodes_alive_time);
    free(known_nodes_count);

    return 0;
}

void *listen_messages(void *arg)
{
    Parameters *params = (Parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_nodes = params->known_nodes_param;
    size_t *known_nodes_count = params->known_nodes_count_param;

    while (run_threads)
    {
        Packet packet;

        struct sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        if (run_threads)
            recvfrom(socketfd, &packet, sizeof(Packet), MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), ip, INET_ADDRSTRLEN);

        uint16_t sender_port = ntohs(cliaddr.sin_port);

        if (packet.type == MSG_TYPE_ORDINARY)
        {
            // printf("got message with length of %ld\n", packet.len);
            printf("sender address: %s:%d and he send this: %s\n", ip, sender_port, packet.data.message);
        }
        else if (packet.type == MSG_TYPE_HEARTBEAT)
        {
            if (*known_nodes_count == MAXNODES)
            {
                printf("this node has maximum count of known nodes!\n");
                continue;
            }

            size_t number_of_nodes = packet.len;

            // printf("got hearbeat with %ld nodes\n", number_of_nodes);

            uint16_t *received_ports = (uint16_t *)calloc(len, sizeof(uint16_t));

            memcpy(received_ports, packet.data.ports, len * sizeof(uint16_t));

            merge_known_ports(known_nodes, known_nodes_count, received_ports, packet.len);
            *known_nodes_count = get_count(known_nodes);

            free(received_ports);
        }
        else
        {
            printf("cannot distinguish message type");
        }
    }
    pthread_exit(NULL);
    return NULL;
}

void send_message(int socketfd, char *message, uint16_t *known_nodes, size_t known_nodes_count)
{
    for (size_t i = 0; i < known_nodes_count; i++)
    {
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(known_nodes[i]);

        Packet packet;
        memset(&packet, 0, sizeof(Packet));

        packet.type = MSG_TYPE_ORDINARY;
        packet.len = strlen(message);
        memcpy(&packet.data.message, message, packet.len * sizeof(char));

        sendto(socketfd, &packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
}

void *send_heartbeat(void *arg)
{
    Parameters *params = (Parameters *)arg;
    int socketfd = params->socketfd_param;
    uint16_t *known_nodes = params->known_nodes_param;
    size_t *known_nodes_count = params->known_nodes_count_param;

    while (run_threads)
    {
        sleep(HEARTBEAT_TIMER);

        for (size_t i = 0; i < *known_nodes_count; i++)
        {
            struct sockaddr_in servaddr;

            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = INADDR_ANY;
            servaddr.sin_port = htons(known_nodes[i]);

            // printf("sending heartbeat from %d to %d\n", port, known_nodes[i]);

            Packet packet;

            memset(&packet, 0, sizeof(Packet));

            packet.type = MSG_TYPE_HEARTBEAT;
            packet.len = *known_nodes_count + 1;

            memcpy(&packet.data.ports, known_nodes, packet.len * sizeof(uint16_t));
            memcpy(&packet.data.ports[*known_nodes_count], &port, sizeof(uint16_t));

            if (run_threads)
                sendto(socketfd, &packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

            // printf("send %ld of bytes where packet is %ld\n", send_size, sizeof(packet));
        }
    }

    pthread_exit(NULL);
    return NULL;
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

void print_known_nodes(uint16_t *known_nodes, size_t known_nodes_count)
{
    for (size_t i = 0; i < known_nodes_count; i++)
    {
        printf("\tknown node: %d\n", known_nodes[i]);
    }
}

void merge_known_ports(uint16_t *known_nodes, size_t *known_nodes_count, uint16_t *received_ports, size_t received_ports_count)
{
    for (size_t i = 0; i < received_ports_count; i++)
    {
        if (received_ports[i] == port || received_ports[i] == 0)
            continue;
        bool is_present = false;
        for (size_t j = 0; j < *known_nodes_count; j++)
        {
            if (known_nodes[j] == received_ports[i])
            {
                is_present = true;
                break;
            }
        }
        if (!is_present)
        {
            append(known_nodes, known_nodes_count, received_ports[i]);
        }
    }
}

void append(uint16_t *array, size_t *length, uint16_t port)
{
    for (size_t i = 0; i < MAXNODES; i++)
    {
        if (array[i] == 0)
        {
            array[i] = port;
            *length += 1;
            return;
        }
    }
    printf("cannot append to array with maximum length");
}

size_t get_count(uint16_t *array)
{
    size_t count = 0;
    for (size_t i = 0; i < MAXNODES; i++)
    {
        if (array[i] != 0)
        {
            count++;
        }
    }
    return count;
}