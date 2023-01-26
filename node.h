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
#define HEARTBEAT_TIMER 1
#define MAX_TIMEOUT 5

#define MSG_TYPE_HEARTBEAT 0
#define MSG_TYPE_ORDINARY 1

typedef union Data
{
    uint16_t ports[MAXNODES + 1];
    char message[1024];
} Data;

typedef struct Packet
{
    uint8_t type;
    size_t len;
    Data data;
} Packet;

typedef struct Parameters
{
    int socketfd_param;
    uint16_t *known_nodes_param;
    struct timeval *known_nodes_alive_time_param;
    size_t *known_nodes_count_param;
} Parameters;

int create_socket_and_bind(int port);
size_t get_count(uint16_t *array);
size_t append(uint16_t *array, size_t *length, uint16_t port);
void *listen_messages(void *arg);
void merge_known_ports(uint16_t *known_nodes, size_t *known_nodes_count, uint16_t *received_ports);
void print_known_nodes(uint16_t *known_nodes);
void *send_heartbeat(void *arg);
void send_message(int socketfd, char *message, uint16_t *known_nodes, size_t known_nodes_count);
void reduce_dead_nodes(uint16_t *known_nodes, size_t *known_nodes_count, struct timeval *known_nodes_alive_time);
void update_response_time(uint16_t *known_nodes, uint16_t sender_port, struct timeval *known_nodes_alive_time);

#endif