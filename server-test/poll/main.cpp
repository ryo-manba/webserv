#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <map>
#include <set>
#include <vector>
#include <iostream>

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
