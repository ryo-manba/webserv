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
#include <sstream>

class Server
{
private:
    // member variables
    bool is_close_connection;
    bool is_compress;

    // 接続要求受け取りに使う
    std::set<int> listen_fds;
    // すべてのfd
    std::vector<pollfd> poll_fds;

    // fd, 受け取ったデータ
    std::map<int, std::string> received_data;

public:
    // methods
    Server(void);
    ~Server(void);

    void init(void);
    void init_pollfds(void);

    void start(void);

    // 接続受け付け
    void listen_socket(const char *port);
    int open_listen_fd(char *port);
    void accept_connection(int fd);

    void polling(void);
    void active_fds(std::vector<pollfd>::iterator it);
    void compress_array(void);
    void receive_and_concat_data(std::vector<pollfd>::iterator it);

    // レスポンスの生成()
    std::string create_response(void);

    // レスポンスをデータにエンコード
//    write_buffer_t  encode_to_write_data(response_t response);


    // データの送信
    void send_data(std::vector<pollfd>::iterator it);

};
