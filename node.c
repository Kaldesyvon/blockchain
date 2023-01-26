#include "node.h"

static bool run_threads = true;
static uint16_t port;

int main(const int argc, const char *argv[])
{
    port = atoi(argv[1]);
    int port_to_connect = atoi(argv[2]);

    int socketfd;
    Parameters params;

    uint16_t *known_nodes = (uint16_t *)malloc(MAX_NODES * sizeof(uint16_t));
    memset(known_nodes, 0, MAX_NODES * sizeof(uint16_t));

    struct timeval *known_nodes_alive_time = (struct timeval *)malloc(MAX_NODES * sizeof(struct timeval));
    memset(known_nodes_alive_time, 0, MAX_NODES * sizeof(struct timeval));

    size_t *known_nodes_count = (size_t *)malloc(sizeof(size_t));
    *known_nodes_count = 0;

    if (port_to_connect != 0)
    {
        known_nodes[*known_nodes_count] = port_to_connect;
        *known_nodes_count += 1;

        struct timeval now;
        gettimeofday(&now, NULL);
        known_nodes_alive_time[0] = now;

        // printf("inital node known at: %ld\n", known_nodes_alive_time[0].tv_sec);
    }

    socketfd = create_socket_and_bind(port);

    params.socketfd_param = socketfd;
    params.known_nodes_param = known_nodes;
    params.known_nodes_alive_time_param = known_nodes_alive_time;
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
            send_message(socketfd, message, known_nodes);
        }
        else if (strcmp(input, "print") == 0)
        {
            if (*known_nodes_count == 0)
            {
                printf("\tno node is known\n");
            }
            print_known_nodes(known_nodes);
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
    struct timeval *known_nodes_alive_time = params->known_nodes_alive_time_param;
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
            printf("got heartbeat from %d\n", sender_port);
            if (*known_nodes_count == MAX_NODES)
            {
                printf("this node has maximum count of known nodes!\n");
                continue;
            }

            // size_t number_of_nodes = packet.len;

            // printf("got hearbeat with %ld nodes\n", number_of_nodes);

            uint16_t *received_ports = (uint16_t *)calloc(MAX_NODES, sizeof(uint16_t));

            memcpy(received_ports, packet.data.ports, MAX_NODES * sizeof(uint16_t)); // potiential error with index out of range

            merge_known_ports(known_nodes, known_nodes_count, received_ports);

            free(received_ports);
        }
        else
        {
            printf("cannot distinguish message type");
        }
        update_response_time(known_nodes, sender_port, known_nodes_alive_time);
    }
    pthread_exit(NULL);
    return NULL;
}

void send_message(int socketfd, char *message, uint16_t *known_nodes)
{
    for (size_t i = 0; i < MAX_NODES && known_nodes[i] != 0; i++)
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
    struct timeval *known_nodes_alive_time = params->known_nodes_alive_time_param;
    size_t *known_nodes_count = params->known_nodes_count_param;

    while (run_threads)
    {
        sleep(HEARTBEAT_TIMER);

        reduce_dead_nodes(known_nodes, known_nodes_count, known_nodes_alive_time);

        for (size_t i = 0; i < MAX_NODES; i++)
        {
            if (known_nodes[i] != 0)
            {
                printf("sending heartbeat to %d\n", known_nodes[i]);

                struct sockaddr_in servaddr;

                servaddr.sin_family = AF_INET;
                servaddr.sin_addr.s_addr = INADDR_ANY;
                servaddr.sin_port = htons(known_nodes[i]);

                Packet packet;

                memset(&packet, 0, sizeof(Packet));

                packet.type = MSG_TYPE_HEARTBEAT;
                packet.len = *known_nodes_count + 1;

                memcpy(&packet.data.ports, known_nodes, MAX_NODES * sizeof(uint16_t));
                memcpy(&packet.data.ports[*known_nodes_count], &port, sizeof(uint16_t));

                if (run_threads)
                    sendto(socketfd, &packet, sizeof(packet), MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

                // printf("send %ld of bytes where packet is %ld\n", send_size, sizeof(packet));
            }
        }
    }

    pthread_exit(NULL);
    return NULL;
}

void reduce_dead_nodes(uint16_t *known_nodes, size_t *known_nodes_count, struct timeval *known_nodes_alive_time)
{
    for (size_t i = 0; i < MAX_NODES; i++)
    {
        if (known_nodes[i] != 0 && known_nodes_alive_time[i].tv_sec != 0)
        {
            struct timeval now;
            gettimeofday(&now, NULL);

            printf("now: %ld last response %ld\n", now.tv_sec, known_nodes_alive_time[i].tv_sec);

            if (now.tv_sec - known_nodes_alive_time[i].tv_sec > MAX_TIMEOUT)
            {
                printf("removing %d\n", known_nodes[i]);

                known_nodes[i] = 0;
                *known_nodes_count -= 1;
                memset(&known_nodes_alive_time[i], 0, sizeof(struct timeval));
            }
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

void print_known_nodes(uint16_t *known_nodes)
{
    for (size_t i = 0; i < MAX_NODES; i++)
    {
        if (known_nodes[i] != 0)
        {
            printf("\tknown node: %d\n", known_nodes[i]);
        }
    }
}

void merge_known_ports(uint16_t *known_nodes, size_t *known_nodes_count, uint16_t *received_ports)
{
    for (size_t i = 0; i < MAX_NODES; i++)
    {
        if (received_ports[i] != port && received_ports[i] != 0)
        {
            bool is_present = false;
            for (size_t j = 0; j < MAX_NODES; j++)
            {
                if (known_nodes[j] == received_ports[i])
                {
                    is_present = true;
                    break;
                }
            }
            if (!is_present)
            {
                size_t appended_position = append(known_nodes, known_nodes_count, received_ports[i]);
            }
        }
    }
}

void update_response_time(uint16_t *known_nodes, uint16_t sender_port, struct timeval *known_nodes_alive_time)
{
    for (size_t i = 0; i < MAX_NODES; i++)
    {
        if (known_nodes[i] != 0 && known_nodes[i] == sender_port)
        {
            struct timeval now;
            gettimeofday(&now, NULL);

            memcpy(&known_nodes_alive_time[i], &now, sizeof(struct timeval));

            // printf("updated %d port with response time of %ld\n", received_ports[i], known_nodes_alive_time[j].tv_sec);
        }
    }
}

size_t append(uint16_t *array, size_t *length, uint16_t port)
{
    for (size_t i = 0; i < MAX_NODES; i++)
    {
        if (array[i] == 0)
        {
            array[i] = port;
            *length += 1;
            return i;
        }
    }
    printf("cannot append to array with maximum length");
}

size_t get_count(uint16_t *array)
{
    size_t count = 0;
    for (size_t i = 0; i < MAX_NODES; i++)
    {
        if (array[i] != 0)
        {
            count++;
        }
    }
    return count;
}