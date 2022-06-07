#include "test_common.hpp"
#include "Server.hpp"

int main()
{
    Server server;

    server.listen("8080");
    server.listen("8081");
    server.listen("8082");
    server.listen("8083");
    server.listen("8084");
    server.init();
    server.start();
}
