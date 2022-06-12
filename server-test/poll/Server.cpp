#include "Server.hpp"
#include "test_common.hpp"

void error_exit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

Server::Server(void)
{
}

Server::~Server(void)
{
    for (std::vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); it++)
    {
        if (it->fd >= 0)
            close(it->fd);
    }
}

int Server::open_listen_fd(char *port)
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
    if (::listen(fd, 1024) < 0)
    {
        return -1;
    }
    return fd;
}

void Server::listen_socket(const char *port)
{
    int fd = open_listen_fd((char *)port);
    if (fd < 0)
        error_exit("open listen failed");
    listen_fds.insert(fd);
}

void Server::init(void)
{
    int on = 1;
    for (std::set<int>::iterator it = listen_fds.begin(); it != listen_fds.end(); it++)
    {
        // fdをノンブロッキングに設定する(fcntlを使う)
        int rc = ioctl(*it, FIONBIO, (char *)&on);
        if (rc < 0)
            error_exit("ioctl() failed");

        pollfd pfd;
        pfd.fd = *it;
        pfd.events = POLLIN;

        // 監視用のfdをセットする
        poll_fds.push_back(pfd);
    }
}

void Server::polling(void)
{
    DOUT() << ("Waiting on poll()...") << std::endl;

    //    int rc = poll(&*poll_fds.begin(), poll_fds.size(), 100000);
    int rc = poll(&*poll_fds.begin(), poll_fds.size(), -1);

    if (rc < 0)
    {
        error_exit("poll() failed");
    }
    if (rc == 0)
    {
        error_exit("poll() timed out.  End program.");
    }
}

void Server::accept_connection(int fd)
{
    // DOUT() << "Listening socket is readable" << std::endl;
    // リッスン待ちのfdをすべて受け入れる
    while (1)
    {
        int new_sd = accept(fd, NULL, NULL);
        if (new_sd < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                error_exit("accept() failed");
            }
            // DOUT() << "===============EWOULDBLOCK================" << std::endl;
            break;
        }

        pollfd pfd;
        pfd.fd = new_sd;
        //        pfd.events = (POLLIN | POLLOUT);
        pfd.events = POLLIN;

        // active socketを追加する
        poll_fds.push_back(pfd);
        // DOUT() << "poll_fds.push_back(new_fd)" << std::endl;
    }
}

void Server::receive_and_concat_data(std::vector<pollfd>::iterator it)
{
    char buf[1024];
    while (1)
    {
        // すべてのデータを受け取る
        int rc = recv(it->fd, buf, sizeof(buf), 0);
        if (rc < 0)
        {
            if (errno != EWOULDBLOCK)
                error_exit("recv() failed");

            DOUT() << "finished receive" << std::endl;
            for (std::vector<pollfd>::iterator it2 = poll_fds.begin(); it2 != poll_fds.end(); it2++)
            {
                if (it2->fd == it->fd)
                {
                    it2->events |= POLLOUT;
                }
            }
            break;
        }
        // recvの戻り値が0の場合 connectionを切断する
        if (rc == 0)
        {
            DSOUT() << "Connection closed" << std::endl;
            DSOUT() << received_data[it->fd] << std::endl;
            received_data[it->fd] = "";

            is_close_connection = true;
            break;
        }
        received_data[it->fd] += std::string(buf, rc);
    }
    // データの受信が終わったのでfdをクローズする
    if (is_close_connection == true)
    {
        close(it->fd);
        it->fd = -1;
        is_compress = true;
    }
}

std::string Server::create_response(void)
{
    std::stringstream ss;

    // status line
    ss << "HTTP/1.1 " << 200 << " "
       << "OK"
       << "\r\n";

    // header
    ss << "Date: Sun, 12 Jun 2022 13:23:58 GMT"
       << "\r\n";
    ss << "Content-Length: 5\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Connection: keep-alive\r\n";
    ss << "Last-Modified: Thu, 27 Feb 2020 07:10:16 GMT\r\n";
    ss << "ETag: \"21a-59f8969af8600\"\r\n";
    ss << "Accept-Ranges: bytes\r\n";
    ss << "Server: Apache\r\n";
    ss << "\r\n";

    // message
    ss << "hello";

    return ss.str();
}

void Server::send_data(std::vector<pollfd>::iterator it)
{
    static_cast<void>(it);
    DOUT() << "it->fd: " << it->fd << std::endl;
    DOUT() << "send message:" << std::endl;
    DOUT() << create_response() << std::endl;
    std::string resp = create_response();
    size_t len = resp.size();
    send(it->fd, resp.c_str(), len, 0);
    for (std::vector<pollfd>::iterator it2 = poll_fds.begin(); it2 != poll_fds.end(); it2++)
    {
        if (it2->fd == it->fd)
        {
            it2->events = POLLIN;
        }
    }
}

void Server::active_fds(std::vector<pollfd>::iterator it)
{
    is_close_connection = false;
    if (it->revents & POLLIN)
    {
        receive_and_concat_data(it);
    }
    if (it->revents & POLLOUT)
    {
        send_data(it);
    }
}

void Server::compress_array(void)
{
    is_compress = false;
    for (std::vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end();)
    {
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

void Server::start(void)
{
    DOUT() << "Server started" << std::endl;
    while (1)
    {
        polling();

        for (std::vector<pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); it++)
        {
            // reventsに変化がない場合はcontinue
            if (it->revents == 0)
                continue;

            // 読み込み、書き込み以外のflagが立っている場合
            if ((it->revents & (POLLIN | POLLOUT)) == 0)
                error_exit("revents error");

            // listenの場合
            if (listen_fds.find(it->fd) != listen_fds.end())
            {
                accept_connection(it->fd);
                break;
            }
            else // 実際にやり取りを行う
            {
                active_fds(it);
            }
        }
        // 削除処理
        if (is_compress == true)
        {
            compress_array();
        }
    }
}
