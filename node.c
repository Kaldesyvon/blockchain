#include "node.h"

static uint16_t port;

int main(const int argc, const char *argv[]) // todo: add alive nodes list that newly created thread will count aliveness of node and if trehsold is exceded, than remove from known nodes
{
    port = atoi(argv[1]);
    uint16_t port_to_connect = atoi(argv[2]);

    int socketfd;
    Parameters params;

    Known_Node *known_ports = (Known_Node *)calloc(MAXNODES + 1, sizeof(Known_Node)); // ensure that node has room for its own port to send over heartbeat
    size_t *known_ports_count = (size_t *)calloc(1, sizeof(size_t));

    if (port_to_connect != 0)
    {
        known_ports[*known_ports_count].port = port_to_connect;

        struct timeval time_of_node_creation;
        gettimeofday(&time_of_node_creation, NULL);

        known_ports[*known_ports_count].time_of_response = time_of_node_creation;
        *known_ports_count += 1;
        printf("node created at %lds %ldus\n", known_ports[0].time_of_response.tv_sec, known_ports[0].time_of_response.tv_usec);
    }

    socketfd = create_socket_and_bind(port);

    params.socketfd_param = socketfd;
    params.known_ports_param = known_ports;
    params.known_ports_count_param = known_ports_count;

    pthread_t listen_thread_id, send_heartbeat_thread_id;

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
    Known_Node *known_ports = params->known_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    while (1)
    {
        Packet packet;

        struct sockaddr_in cliaddr;
        uint len;

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
            printf("got heartbeat from: %d\n", sender_port);
            if (*known_ports_count == MAXNODES)
            {
                printf("this node has maximum count of known nodes!\n");
                continue;
            }

            Known_Node *received_ports = (Known_Node *)calloc(MAXNODES, sizeof(Known_Node));

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

void send_message(int socketfd, char *message, Known_Node *known_ports, size_t known_ports_count)
{
    for (size_t i = 0; i < known_ports_count; i++)
    {
        struct sockaddr_in servaddr;

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(known_ports[i].port);

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
    Known_Node *known_ports = params->known_ports_param;
    size_t *known_ports_count = params->known_ports_count_param;

    while (1)
    {
        sleep(HEARTBEAT_TIMER);

        for (size_t i = 0; i < *known_ports_count; i++)
        {
            struct sockaddr_in servaddr;

            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = INADDR_ANY;
            servaddr.sin_port = htons(known_ports[i].port);

            Packet packet;

            packet.type = MSG_TYPE_HEARTBEAT;
            memcpy(packet.data.ports, known_ports, sizeof(known_ports));
            append(packet.data.ports, port);

            sendto(socketfd, &packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

            printf("heartbeat sent to %d\n", known_ports[i].port);
        }
    }

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

void print_known_nodes(Known_Node *known_ports, size_t known_ports_count)
{
    for (size_t i = 0; i < known_ports_count; i++)
    {
        printf("\tknown node: %d\n", known_ports[i].port);
    }
}

void merge_known_ports(Known_Node *known_ports, size_t known_ports_count, Known_Node *received_ports, size_t received_ports_count)
{
    for (size_t i = 0; i < received_ports_count; i++)
    {
        if (received_ports[i].port == port)
            continue;
        bool is_present = false;
        for (size_t j = 0; j < known_ports_count; j++)
        {

            if (known_ports[j].port == received_ports[i].port)
            {
                is_present = true;
                break;
            }
        }
        if (!is_present)
        {
            append(known_ports, received_ports[i].port);
        }
    }
}

void append(Known_Node *array, uint16_t port)
{
    size_t size = get_length(array);
    if (size < MAXNODES)
    {
        array[size].port = port;
        gettimeofday(&array[size].time_of_response, NULL);
    }
    else
    {
        printf("cannot append to array with maximum length");
    }
}

size_t get_length(Known_Node *array)
{
    size_t size = 0;
    while (array[size].port != 0 && array[size].time_of_response.tv_sec != 0 && size < MAXNODES)
    {
        size++;
    }
    return size;
}