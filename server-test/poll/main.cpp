#include "test_common.hpp"
#include "Server.hpp"

int main()
{
    Server server;

    server.listen_socket("8080");
    server.listen_socket("8081");
    server.listen_socket("8082");
    server.listen_socket("8083");
    server.listen_socket("8084");
    server.init();
    server.start();
}
