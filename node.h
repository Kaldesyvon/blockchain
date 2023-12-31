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
#include <openssl/sha.h>

#define MAX_LINE 1024
#define MAX_NODES 6
#define HEARTBEAT_TIMER 1
#define MAX_TIMEOUT 3
#define MAX_TRANSACTIONS 5

#define MSG_TYPE_HEARTBEAT 0
#define MSG_TYPE_ORDINARY 1
#define MSG_TYPE_TRANSACTION 2
#define MSG_TYPE_BLOCK 3
#define MSG_TYPE_BLOCK_FORCED 4
#define MSG_TYPE_BLOCK_REQUEST 5

typedef struct Transaction
{
    uint16_t port;
    struct timeval timestamp;
    char message[MAX_LINE];
} Transaction;

typedef struct Block
{
    uint64_t index;
    struct timeval timestamp;
    Transaction transactions[MAX_TRANSACTIONS];
    unsigned char pervious_hash[SHA256_DIGEST_LENGTH];
    unsigned char hash[SHA256_DIGEST_LENGTH];
} Block;

typedef union Data
{
    uint16_t ports[MAX_NODES + 1];
    char message[MAX_LINE];
    Transaction transaction;
    Block block;
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

static size_t transaction_count = 0;
static Transaction *transactions;

static size_t blockchain_length = 1;
static Block *blockchain;

bool am_i_block_creator(uint16_t *known_nodes);
size_t append(uint16_t *array, size_t *length, uint16_t port);
void create_block();
int create_socket_and_bind(int port);
void handle_transaction(Transaction *transaction, int socket, uint16_t *known_nodes);
void *listen_messages(void *arg);
void print_blockchain();
void merge_known_ports(uint16_t *known_nodes, size_t *known_nodes_count, uint16_t *received_ports);
void print_known_nodes(uint16_t *known_nodes, size_t known_nodes_count);
void print_transactions();
void reduce_dead_nodes(uint16_t *known_nodes, size_t *known_nodes_count, struct timeval *known_nodes_alive_time);
void *send_heartbeat(void *arg);
void send_message(int socketfd, char *message, uint16_t *known_nodes, int msg_type);
void update_response_time(uint16_t *known_nodes, uint16_t sender_port, struct timeval *known_nodes_alive_time);
// TODO:
bool validate_block(Block *block);
// void write_block_to_file();
// void read_block_from_file();

void request_blocks(int socketfd, uint16_t port_to_request);

#endif