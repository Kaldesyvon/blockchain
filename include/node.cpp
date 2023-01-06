#include <iostream>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "node.h"

struct sockaddr_in my_addr;

Node::Node(std::string name, int port)
{
    name = name;
    port = port;

    socketfd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (socketfd == -1)
    {
        std::cout << "Error creating socket for " << name << " node" << std::endl;
        exit(1);
    }
}

int Node::get_socket()
{
    return socketfd;
}

void Node::add_socket(int socketfd)
{
    sockets.push_back(socketfd);
}

void Node::broadcast(std::string message)
{
    for (int socket : sockets)
    {
        my_addr.sin_family = AF_UNIX; // naplnenie struktury sockaddr_in
        my_addr.sin_port = htons(port);
        my_addr.sin_addr.s_addr = INADDR_ANY;
        bzero(&(my_addr.sin_zero), 8);

        if (bind(socket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
        {
            std::cout << "Could not bind onto socket " << socket << std::endl;
            exit(1);
        }

        // if (send(int s, const void *msg, size_t len, int flags) == -1) {
        //     std::cout << "Could not send from " << socketfd << " to socket " << socket << std::endl;
        //     exit(1);
        // }
    }
}

void Node::recieve()
{
}