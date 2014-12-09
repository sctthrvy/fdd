#ifndef _FD_H
#define _FD_H 1

#include <sys/types.h>
#include <sys/socket.h> /* For the PF_*, AF_*, SOCK_*, ... constants */

/* Acts like socket(2) */
extern int socketfd(int domain, int type, int protocol);

/**
* Create n sockets with a call to socket(domain, type, protocol), storing the
* sockets starting at fdbuf.
*
* Returns: 0 on success or -1 on failure (in which case no sockets are left
*          open).
*/
extern int socketfds(int domain, int type, int protocol, int *fdbuf, int n);

/* Acts like socketpair(2) */
extern int socketpairfd(int domain, int type, int protocol, int sv[2]);

/* Acts like open(2) */
extern int openfd(const char *pathname, int flags);

/* Acts like pipe(2) */
extern int pipefd(int pipefd[2]);

#endif /* _FD_H */
