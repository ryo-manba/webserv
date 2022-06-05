#include "Server.hpp"
#include "utility.hpp"
#include <iostream>
#include "test_common.hpp"

Server::Server() : listens_(0),
                   maxfd_(-1),
                   maxi_(-1),
                   nready_(0),
                   client_fds_(FD_SETSIZE, -1),
                   listen_fds_(FD_SETSIZE, -1),
                   requests_(FD_SETSIZE)
{
    std::cout << "Server started" << std::endl;
}

Server::~Server()
{
}

void Server::listen(const char *port)
{
    if ((listen_fds_[listens_] = open_listenfd((char *)port)) < 0)
        unix_error("open_listenfd error");
    listens_ += 1;
}

void Server::init()
{
    for (int i = 0; i < listens_; i++)
    {
        if (maxfd_ < listen_fds_[i])
            maxfd_ = listen_fds_[i];
    }
    FD_ZERO(&read_set_);
    for (int i = 0; i < listens_; i++)
    {
        FD_SET(listen_fds_[i], &read_set_);
    }
}

void Server::run()
{
    while (1)
    {
        ready_set_ = read_set_;
        nready_ = Select(maxfd_ + 1, &ready_set_, NULL, NULL, NULL);
        for (int i = 0; i < listens_; i++)
        {
            if (FD_ISSET(listen_fds_[i], &ready_set_))
            {
                socklen_t clientlen = sizeof(struct sockaddr_storage);
                struct sockaddr_storage clientaddr;
                int connfd = Accept(listen_fds_[i], (struct sockaddr *)&clientaddr, &clientlen);
                // clientを追加する
                add_client(connfd);
            }
        }
        check_clients();
    }
}

void Server::add_client(int connfd)
{
    int i;
    nready_--;
    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (client_fds_[i] < 0)
        {
            // コネクト済みのfdを追加する
            client_fds_[i] = connfd;

            FD_SET(connfd, &read_set_);

            if (connfd > maxfd_)
                maxfd_ = connfd;
            if (i > maxi_)
                maxi_ = i;
            break;
        }
        // 空のスロットが見つからなかった場合
        if (i == FD_SETSIZE)
        {
            app_error("add_client error: Too many clients");
        }
    }
}

void Server::check_clients()
{
    int i, connfd, n;
    char buf[MAXLINE];

    for (i = 0; (i <= maxi_) && (nready_ > 0); i++)
    {
        connfd = client_fds_[i];
        // ディスクリプタが準備できてた場合にreadする
        if ((connfd > 0) && (FD_ISSET(connfd, &ready_set_)))
        {
            nready_--;
            if ((n = read(connfd, buf, MAXLINE)) != 0)
            {
                requests_[i] += std::string(buf);
                memset(buf, 0, sizeof(buf));
            }
            else
            { // EOFが見つかった場合除去する
                std::cout << requests_[i] << std::endl;
                close(connfd);
                FD_CLR(connfd, &read_set_);
                client_fds_[i] = -1;
            }
        }
    }
}

int Server::open_listenfd(char *port)
{
    struct addrinfo hints, *listp, *p;
    int fd, optval = 1;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_flags |= AI_NUMERICSERV;
    hints.ai_flags |= AI_ADDRCONFIG;
    getaddrinfo(NULL, port, &hints, &listp);

    // listすべての初期設定
    for (p = listp; p; p = p->ai_next)
    {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
            continue;

        Setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if (bind(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(fd);
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;

    // 接続要求を受け付けるsocket
    if (::listen(fd, LISTENQ) < 0)
    {
        return -1;
    }
    return fd;
}
