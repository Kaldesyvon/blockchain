#include <iostream>

int main() {
    Node n1 = Node("marcin", 25667);
    Node n2 = Node("boris", 25668);
    n1.add_socket(n2.get_socket());

    n1.broadcast("ay yo");

    return 0;
}