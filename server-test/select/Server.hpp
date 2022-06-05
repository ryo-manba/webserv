#ifndef SERVER_HPP
#define SERVER_HPP

#include "utility.hpp"
#include <string>
#include <vector>

class Server
{
public:
    int maxfd_;
    fd_set read_set_;
    fd_set ready_set_;
    int listens_;
    int nready_;
    int maxi_;

    std::vector<int> client_fds_;
    std::vector<int> listen_fds_;
    std::vector<std::string> requests_;

public:
    Server();
    ~Server();

    void listen(const char *port);
    void init();
    void run();
    void add_client(int connfd);
    void check_clients();
    int open_listenfd(char *port);
};

#endif
