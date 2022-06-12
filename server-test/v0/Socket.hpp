#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 8080
#define SIZE (5 * 1024)

class Socket
{
public:
    int fd;
    struct sockaddr_in a_addr;

    Socket(void) : fd(socket(AF_INET, SOCK_STREAM, 0))
    {
        if (fd == -1)
        {
            throw std::runtime_error("socket error");
        }
    }

    Socket(int fd) : fd(fd)
    {
    }

    ~Socket()
    {
        close(fd);
    }

    void setup()
    {
        // 構造体を全て0にセット
        memset(&a_addr, 0, sizeof(struct sockaddr_in));

        // サーバーのIPアドレスとポートの情報を設定
        a_addr.sin_family = AF_INET;
        a_addr.sin_port = htons((unsigned short)SERVER_PORT);
        a_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

        // ソケットに情報を設定
        if (bind(fd, (const struct sockaddr *)&a_addr, sizeof(a_addr)) == -1)
        {
            throw std::runtime_error("socket error");
        }
    }
};

#endif
