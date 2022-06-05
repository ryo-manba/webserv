#include "Server.hpp"
#include "utility.hpp"

int main(int argc, char **argv)
{
    Server server;
    // if (argc < 2)
    // {
    //     fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    //     exit(0);
    // }
    //    for (int i = 1; i < argc; i++) server.listen(argv[i]);

    server.listen("8080");
    server.listen("8081");
    server.listen("8082");
    server.listen("8083");
    server.listen("8084");

    server.init();
    server.run();
}
