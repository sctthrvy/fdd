#include <ldap.h>
#include "fd.h"
#include "fdcommon.h"
#include "debug.h"


/**
* Uses recvmsg(2) to receive descriptors.
*
* Returns: the number of descriptors received, or
*          -2 on error
*/
int recv_fds(int recvsock, struct sockaddr_un *srcaddr, char *databuf, int datalen, int *fdbuf, int numfds) {
    ssize_t errs;
    struct msghdr msg;
    struct iovec iov[1];
    struct cmsghdr *cmsgp, *tmp_cmsgp;
    int fds_recvd = 0;

    if(datalen <= 0 || numfds <= 0 || !databuf || !fdbuf) {
        error("datalen: %d <= 0 || numfds: %d <= 0 || !(databuf: %p) || !(fdbuf: %p)",
                        datalen, numfds, (void*)databuf, (void*)fdbuf);
        errno = EINVAL;
        return -2;
    }

    cmsgp = malloc(CMSG_SPACE(sizeof(int)) * ((size_t)numfds + 1)); /* space for kernel to store cmsghdr{}'s */
    if(cmsgp == NULL) {
        error("malloc: %s\n", strerror(errno));
        return -2;
    }

    iov[0].iov_base = databuf;
    iov[0].iov_len = (size_t)datalen;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    msg.msg_name = srcaddr;
    msg.msg_namelen = sizeof(struct sockaddr_un);

    msg.msg_control = cmsgp;
    msg.msg_controllen = 100;

    msg.msg_flags = 0;

    errs = recvmsg(recvsock, &msg, 0);
    if(errs < 0) {
        error("recvmsg: %m\n");
        free(cmsgp);
        return -2;
    }
    debug("msg recvd: %d bytes, msg_fls\n", (int)errs)
    if(msg.msg_controllen < sizeof(struct cmsghdr)) {
        error("Kernel didn't fill cmsg? msg.controllen: %ju\n", (uintmax_t)msg.msg_controllen);
        free(cmsgp);
        return -2;
    }

    for(tmp_cmsgp = CMSG_FIRSTHDR(&msg); tmp_cmsgp != NULL; tmp_cmsgp = CMSG_NXTHDR(&msg, tmp_cmsgp)) {
        debug("Processing cmsg: ")
        if(
                tmp_cmsgp->cmsg_level == SOL_SOCKET &&
                tmp_cmsgp->cmsg_type == SCM_RIGHTS) {
            *(fdbuf + fds_recvd) = *((int *) CMSG_DATA(tmp_cmsgp));
            fds_recvd++;
        }
    }

    free(cmsgp);
    return (int)errs;
}

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
int socketfd(int domain, int type, int protocol) {
    struct fdreq req;
    int fddsock, usersock = -1;
    ssize_t n;
    struct sockaddr_un fddaddr = { /* fill out fdd's location */
            .sun_family = AF_LOCAL,
            .sun_path = FDD_SOCK_PATH
    };
    socklen_t slen = SUN_LEN(&fddaddr);

    /* Create socket to talk to fdd */
    fddsock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(fddsock < 0) {
        error("socket: %s\n", strerror(errno));
        return -2;
    }

    /* fill out the request */
    req.fdcode = FDCODE_SOCKET;
    req.numfds = 1;
    req.fdreq_domain = domain;
    req.fdreq_type = type;
    req.fdreq_protocol = protocol;

    n = sendto(fddsock, &req, FDREQ_LEN(&req), 0, (struct sockaddr*)&fddaddr, slen);
    if(n < 0) {
        error("sendto: %s\n", strerror(errno));
        return -2;
    }

    /* recvfd() */

    return usersock;
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
