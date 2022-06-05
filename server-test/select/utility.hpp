#ifndef CSAPP_HPP
#define CSAPP_HPP

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

//void init_pool(pool *);
//void add_client(int, pool *);
//void check_clients(pool *);
int Open_listenfd(char *);
int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int Select(int n, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);
void unix_error(const char *msg);
void app_error(const char *msg);


#endif

