#ifndef _FD_H
#define _FD_H 1

#include <sys/types.h>
#include <sys/socket.h> /* For the PF_*, AF_*, SOCK_*, ... constants */

/**
*  Acts like socket(2).
*  Sends request to the fd daemon to create a socket.
*  Blocks until receiving a response.
*
*  Returns: the socket descriptor, or
*           -1 if the user's socket() call failed
*           -2 if socketfd() failed for an internal reason
*           On either case errno is set appropriately.
*/
extern int socketfd(int domain, int type, int protocol);

/* Acts like socketpair(2) */
extern int socketpairfd(int domain, int type, int protocol, int sv[2]);

/* Acts like open(2) */
extern int openfd(const char *pathname, int flags);

/* Acts like pipe(2) */
extern int pipefd(int pipefd[2]);

#endif /* _FD_H */
