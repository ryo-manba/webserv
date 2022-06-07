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
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Socket.hpp"

#define TRUE 1
#define FALSE 0

using namespace std;

class Server
{
public:
    char buffer[1028];
    bool close_conn;
    bool compress_array;

    set<int> listen_fds;

    vector<pollfd> poll_fds;

    vector<Socket*> sockets;


    // fd, 受け取ったデータ
    map<int, std::string> received_buffers;

    // methods
    Server();
    ~Server();

    void init_pollfds();
    void init();

    void run();
    void start();

    int open_listenfd(char *port);
    void listen(const char *port);
    void polling();
    void accept_fds(int fd);
    void active_fds(vector<pollfd>::iterator it);
    void receive(vector<pollfd>::iterator it);

};
