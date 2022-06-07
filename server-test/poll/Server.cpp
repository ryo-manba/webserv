#include "Server.hpp"
#include "test_common.hpp"

void error_exit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

Server::Server()
{
}

Server::~Server()
{
    for (vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); it++)
    {
        if (it->fd >= 0)
            close(it->fd);
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

        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));

        if (::bind(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(fd);
    }

    freeaddrinfo(listp);
    if (!p)
        return -1;

    // 接続要求を受け付けるsocket
    if (::listen(fd, 10) < 0)
    {
        return -1;
    }
    return fd;
}

void Server::listen(const char *port)
{
    int fd = open_listenfd((char *)port);
    if (fd < 0)
        error_exit("open listen failed");
    // listenのみで使われるset
    listen_fds.insert(fd);
}

void Server::init()
{
    int on = 1;
    for (set<int>::iterator it = listen_fds.begin(); it != listen_fds.end(); it++)
    {
        // fdをノンブロッキングに設定する
        int rc = ioctl(*it, FIONBIO, (char *)&on);
        if (rc < 0)
            error_exit("ioctl() failed");

        pollfd pfd;
        pfd.fd = *it;
        pfd.events = POLLIN;
        poll_fds.push_back(pfd);
    }
}

void Server::polling()
{
    // DOUT() << ("Waiting on poll()...") << std::endl;

    //    int rc = poll(&*poll_fds.begin(), poll_fds.size(), 100000);
    int rc = poll(&*poll_fds.begin(), poll_fds.size(), -1);

    if (rc < 0)
        error_exit("poll() failed");
    if (rc == 0)
        error_exit("poll() timed out.  End program.");
}

void Server::accept_fds(int fd)
{
    // DOUT() << "Listening socket is readable" << std::endl;
    // リッスン待ちのfdをすべて受け入れる
    while (1)
    {
        int new_sd = accept(fd, NULL, NULL);
        if (new_sd < 0)
        {
            if (errno != EWOULDBLOCK)
                error_exit("accept() failed");
            // DOUT() << "===============EWOULDBLOCK================" << std::endl;
            break;
        }

        // DOUT() << "New incoming connection - " << new_sd << std::endl;

        pollfd pfd;
        pfd.fd = new_sd;
        // 最初はPOLLIN
        pfd.events = (POLLIN | POLLOUT);
        poll_fds.push_back(pfd);
        // DOUT() << "poll_fds.push_back(new_fd)" << std::endl;
    }
}

// void Server::receive(vector<pollfd>::iterator it)
// {
//     close_conn = false;
//     while (1)
//     {
//         // すべてのデータを受け取る
//         int rc = recv(it->fd, buffer, sizeof(buffer), 0);
//         if (rc < 0)
//         {
//             if (errno != EWOULDBLOCK)
//                 error_exit("recv() failed");
//             DSOUT() << received_buffers[it->fd] << std::endl;
//             break;
//         }
//         // recvの戻り値が0の場合 connectionを切断する
//         if (rc == 0)
//         {
// DOUT() << "Connection closed" << std::endl;
//             close_conn = true;
//             break;
//         }

//        DOUT() << "read count : " << rc << std::endl;
//         if (received_buffers.count(it->fd) == 0)
//             received_buffers[it->fd] = "";
//         received_buffers[it->fd] = received_buffers[it->fd] + std::string(buffer, rc);
//        DOUT() << "finish add_buffers" << std::endl;
//     }
//     // データの受信が終わったのでfdをクローズする
//     if (close_conn == true)
//     {
//         close_conn = false;
//         close(it->fd);
//         it->fd = -1;
//         compress_array = true;
//     }
// }

void Server::active_fds(vector<pollfd>::iterator it)
{
    // DOUT() << "fd      : " << it->fd << std::endl;
    // DOUT() << "revents : " << it->revents << std::endl;
    close_conn = false;
    if (it->revents & POLLIN)
    {
        // DOUT() << "========POLL IN=======" << std::endl;
        while (1)
        {
            // すべてのデータを受け取る
            int rc = recv(it->fd, buffer, sizeof(buffer), 0);
            if (rc < 0)
            {
                if (errno != EWOULDBLOCK)
                    error_exit("recv() failed");
                break;
            }
            // recvの戻り値が0の場合 connectionを切断する
            if (rc == 0)
            {
                DSOUT() << "Connection closed" << std::endl;
                DSOUT() << received_buffers[it->fd] << std::endl;
                received_buffers[it->fd] = "";

                close_conn = true;
                break;
            }

            // DOUT() << "read count : " << rc << std::endl;
            received_buffers[it->fd] = received_buffers[it->fd] + std::string(buffer, rc);
            // DOUT() << "finish add_buffers" << std::endl;
        }
        // データの受信が終わったのでfdをクローズする
        if (close_conn == true)
        {
            // DOUT() << "close fd :" << it->fd << std::endl;
            close(it->fd);
            it->fd = -1;
            compress_array = true;
        }
    }
    if (it->revents & POLLOUT)
    {
        //        DSOUT() << "========POLL OUT=======" << std::endl;
        //        DSOUT() << received_buffers[it->fd] << std::endl;
        //        received_buffers[it->fd] = "";
        //        close(it->fd);
        // 削除の動作をどうするか
        //        it->fd = -1;
        //        compress_array = true;
    }
}

void Server::start()
{
    while (1)
    {
        polling();

        for (vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); it++)
        {
            // DOUT() << "fd   : " << it->fd << std::endl;
            if (it->revents == 0)
                continue;

            // 読み込み、書き込み以外のflagが立っている場合
            if ((it->revents & (POLLIN | POLLOUT)) == 0)
                error_exit("revents error");

            // listenの場合
            if (listen_fds.find(it->fd) != listen_fds.end())
            {
                accept_fds(it->fd);
                break;
            }
            else // listenソケットではなかった場合
            {
                active_fds(it);
            }
        }
        // DOUT() << "commpress_array: " << std::boolalpha << compress_array << std::endl;
        // 削除処理
        if (compress_array == true)
        {
            compress_array = false;
            for (vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end();)
            {
                // DOUT() << "delete fd : " << it->fd << std::endl;
                if (it->fd == -1)
                {
                    it = poll_fds.erase(it);
                }
                else
                {
                    it++;
                }
            }
        }
    }
}
