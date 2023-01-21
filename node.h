#ifndef NODE_H
#define NODE_H

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
#include <stdbool.h>

#define MAXLINE 1024
#define MAXNODES 6
#define HEARTBEAT_TIMER 5

#define MSG_TYPE_HEARTBEAT 0
#define MSG_TYPE_ORDINARY 1


typedef struct Known_Node
{
    uint16_t port;
    struct timeval time_of_response;
} Known_Node;

typedef union Data
{
    char message[1024];
    Known_Node* ports;
} Data;

typedef struct Packet
{
    uint8_t type;
    Data data;
} Packet;

typedef struct Parameters
{
    int socketfd_param;
    Known_Node *known_ports_param;
    size_t *known_ports_count_param;
} Parameters;



int create_socket_and_bind(int port);
size_t get_length(Known_Node *array);
void append(Known_Node *array, uint16_t port);
void *listen_messages(void *arg);
void merge_known_ports(Known_Node *known_ports, size_t known_ports_count, Known_Node *received_ports, size_t received_ports_count);
void print_known_nodes(Known_Node *known_ports, size_t known_ports_count);
void *send_heartbeat(void *arg);
void send_message(int socketfd, char *message, Known_Node *known_ports, size_t known_ports_count);

#endif