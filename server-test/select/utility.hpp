#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXLINE 8192 /* Max text line length */
#define MAXBUF 8192  /* Max I/O buffer size */
#define LISTENQ 1024 /* Second argument to listen() */

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
void Setsockopt(int s, int level, int optname, const void *optval, int optlen);

void unix_error(const char *msg);
void app_error(const char *msg);

#endif
