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

class Server
{
public:
    bool is_close_connection;
    bool is_compress;

    // 接続要求受け取りに使う
    std::set<int> listen_fds;
    // すべてのfd
    std::vector<pollfd> poll_fds;

    // fd, 受け取ったデータ
    std::map<int, std::string> received_data;

    // methods
    Server();
    ~Server();

    void init_pollfds();
    void init();

    void start();

    int open_listenfd(char *port);
    void listen(const char *port);

    void polling();
    void accept_fds(int fd);
    void active_fds(std::vector<pollfd>::iterator it);
    void compress_array();
    void receive(std::vector<pollfd>::iterator it);
    void post(std::vector<pollfd>::iterator it);
};
