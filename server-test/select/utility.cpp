#include "utility.hpp"

// error
void unix_error(const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

void app_error(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(0);
}

// wrapper
void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
{
    int rc;
    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
        unix_error("Setsockopt error");
}

int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout)
{
    int rc;

    if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
        unix_error("Select error");
    return rc;
}

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int rc;

    if ((rc = accept(s, addr, addrlen)) < 0)
        unix_error("Accept error");
    return rc;
}
