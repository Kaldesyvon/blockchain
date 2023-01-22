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
#define HEARTBEAT_TIMER 10

#define MSG_TYPE_HEARTBEAT 0
#define MSG_TYPE_ORDINARY 1

typedef union Data
{
    char message[1024];
    uint16_t ports[MAXNODES];
} Data;

typedef struct Packet
{
    uint8_t type;
    Data data;
} Packet;

typedef struct Parameters
{
    int socketfd_param;
    uint16_t *known_nodes_param;
    size_t *known_nodes_count_param;
} Parameters;


int create_socket_and_bind(int port);
size_t get_length(uint16_t *array);
void append(uint16_t *array, uint16_t port);
void *listen_messages(void *arg);
void merge_known_ports(uint16_t *known_nodes, size_t known_nodes_count, uint16_t *received_ports, size_t received_ports_count);
void print_known_nodes(uint16_t *known_nodes, size_t known_nodes_count);
void *send_heartbeat(void *arg);
void send_message(int socketfd, char *message, uint16_t *known_nodes, size_t known_nodes_count);

#endif