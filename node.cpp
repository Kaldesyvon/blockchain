#include <iostream>
#include <strings.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

struct sockaddr_in my_addr;
std::vector<int> sockets;

void bind_to_socket(int socketfd, int port)
{
    my_addr.sin_family = AF_UNIX; // naplnenie struktury sockaddr_in
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(my_addr.sin_zero), 8);

    if (bind(socketfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
    {
        std::cout << "Could not bind onto socket " << socket << std::endl;
        exit(1);
    }
}

int main(int argc, char const *argv[])
{
    int port = std::stoi(argv[1]);
    std::string name;
    int socketfd;

    socketfd = socket(PF_UNIX, SOCK_DGRAM, 0);
    if (socketfd == -1)
    {
        std::cout << "Error creating socket for " << name << " node" << std::endl;
        exit(1);
    }
    bind_to_socket(socketfd, port);

    std::cout << argv[1] << std::endl;
    return 0;
}
