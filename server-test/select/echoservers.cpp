#include "csapp.h"
#include <string>
#include <iostream>

typedef struct
{
    fd_set read;
    fd_set ready;
} t_set;

typedef struct
{
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    t_set sets;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    int listenfd[FD_SETSIZE];
    int listens;
    std::string buf[FD_SETSIZE];
} pool;

void init_pool(pool *);
void add_client(int, pool *);
void check_clients(pool *);
int Open_listenfd(char *);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);

int byte_cnt = 0;

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pool pool;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(0);
    }
    pool.listens = 0;
    for (int i = 1; i < argc; i++)
    {
        listenfd = Open_listenfd(argv[i]);
        pool.listenfd[i - 1] = listenfd;
        pool.listens += 1;
    }
    init_pool(&pool);

    while (1)
    {
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);
        for (int i = 0; i < pool.listens; i++)
        {
            if (FD_ISSET(pool.listenfd[i], &pool.ready_set))
            {
                clientlen = sizeof(struct sockaddr_storage);
                connfd = Accept(pool.listenfd[i], (struct sockaddr *)&clientaddr, &clientlen);
                // clientをpoolに追加する
                add_client(connfd, &pool);
            }
        }
        check_clients(&pool);
    }
}

void init_pool(pool *p)
{
    int i;
    p->maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++)
    {
        p->clientfd[i] = -1;
    }

    p->maxfd = -1;
    for (i = 0; i < p->listens; i++)
    {
        if (p->maxfd < p->listenfd[i])
            p->maxfd = p->listenfd[i];
    }
    FD_ZERO(&p->read_set);
    for (i = 0; i < p->listens; i++)
    {
        FD_SET(p->listenfd[i], &p->read_set);
    }
}

void app_error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

void add_client(int connfd, pool *p)
{
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (p->clientfd[i] < 0)
        {
            // コネクト済みのfdをpoolに追加する
            p->clientfd[i] = connfd;

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;
        }
        // 空のスロットが見つからなかった場合
        if (i == FD_SETSIZE)
        {
            app_error("add_client error: Too many clients");
        }
    }
}

void check_clients(pool *p)
{
    int i, connfd, n;
    char buf[MAXLINE];

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++)
    {
        connfd = p->clientfd[i];

        // ディスクリプタが準備できてた場合にreadする
        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set)))
        {
            p->nready--;
            if ((n = read(connfd, buf, MAXLINE)) != 0)
            {
                byte_cnt += n;
                p->buf[i] += std::string(buf);
                memset(buf, 0, sizeof(buf));
            }
            else
            { // EOFが見つかった場合poolから除去する
                std::cout << p->buf[i] << std::endl;
                close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}
