# dfs-2022-exam

p2p network in C using sockets

# usage

compile it with `make all` and then run `./node <node_port> <node_port_to_connect>` where `<node_port>` is port on which node will be runnning and `<node_port_to_connect>` is node port to connect to and receive messages and heartbeats. Use 0 for initial node

# valgrind check

run `valgrind --track-origins=yes --leak-check=full -s` to check all memory leaks