#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>



class Node {
private:
    int port;
    std::string name;
    int socketfd;

    std::vector<int> sockets;

public:
    Node(std::string name, int port);
    int get_socket();
    void add_socket(int socketfd);
    void broadcast(std::string message);
    void recieve();
};

#endif