#include "fd.h"
#include "fdcommon.h"
#include "debug.h"


/**
* Uses recvmsg(2) to receive descriptors.
*
* TODO: design
*/
int recv_fds(int sock , struct sockaddr_un *srcaddr, socklen_t slen, char *buf, size_t buflen) {
    ssize_t errs;
    struct msghdr msg;
    struct iovec iov;
    struct cmsghdr *cmsgp = malloc(100);
    if(cmsgp == NULL) {
        error("malloc: %m\n");
        return -1;
    }

    iov.iov_base = buf;
    iov.iov_len = buflen;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    msg.msg_name = srcaddr;
    msg.msg_namelen = slen;

    msg.msg_control = cmsgp;
    msg.msg_controllen = 100;

    msg.msg_flags = 0;

    errs = recvmsg(sock, &msg, 0);
    if(errs < 0) {
        error("recvmsg: %m\n");
        free(cmsgp);
        return -1;
    }
    if(msg.msg_controllen < sizeof(struct cmsghdr)) {
        error("Kernel didn't fill in control data, IP_RECVORIGDSTADDR didn't work?\n");
        free(cmsgp);
        return -1;
    }

    free(cmsgp);
    return (int)errs;
}

/**
*  Acts like socket(2).
*  Sends request to the fd daemon to create a socket.
*  Blocks until recv'ing a response.
*
*  Returns: the socket descriptor, same as socket(2)
*/
int socketfd(int domain, int type, int protocol) {
    
    return -1;
}

/**
* Create n sockets with a call to socket(domain, type, protocol), storing the
* sockets starting at fdbuf.
*
* Returns: 0 on success or -1 on failure (in which case no sockets are left
*          open).
*/
int socketfds(int domain, int type, int protocol, int *fdbuf, int n) {
    return -1;
}

/* Acts like socketpair(2) */
int socketpairfd(int domain, int type, int protocol, int sv[2]) {
    return -1;
}

/* Acts like open(2) */
int openfd(const char *pathname, int flags) {
    return -1;
}

/* Acts like pipe(2) */
int pipefd(int pipefd[2]) {
    return -1;
}
