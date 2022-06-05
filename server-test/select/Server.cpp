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
    listen_fds_[listens_] = Open_listenfd((char *)port);
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
            { // EOFが見つかった場合poolから除去する
                std::cout << requests_[i] << std::endl;
                close(connfd);
                FD_CLR(connfd, &read_set_);
                client_fds_[i] = -1;
            }
        }
    }
}

int Server::Select(int n, fd_set *readfds, fd_set *writefds,
                   fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
        unix_error("Select error");
    return rc;
}

int Server::Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}
