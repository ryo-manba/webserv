#include "Server.hpp"
#include "Socket.hpp"

int main()
{
    Server server;

    server.setup();

    while (true)
    {
        server.run();
    }
    return 0;
}
