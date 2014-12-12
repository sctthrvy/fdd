#include "fd.h"
#include "fdcommon.h"
#include "debug.h"


/**
* Uses recvmsg(2) to receive descriptors.
* Data is stored into databuf, the file descriptors are stored into fdbuf.
* srcaddr can be NULL if you are not interested in the source address.
*
* Returns: number of data bytes received, or
*          -2 on error
*/
static int recv_fds(int recvsock, struct sockaddr_un *srcaddr, void *databuf,
        int datalen, int *fdbuf, int numfds) {

    ssize_t errs;
    struct msghdr msg;
    struct iovec iov[1];
    struct cmsghdr *cmsgp, *tmp_cmsgp;
    size_t cmsgspace;

    if(datalen <= 0 || numfds <= 0 || !databuf || !fdbuf) {
        error("datalen=%d <= 0 || numfds=%d <= 0 || !(databuf=%p) ||"
                "!(fdbuf=%p)",
                        datalen, numfds, databuf, (void*)fdbuf);
        errno = EINVAL;
        return -2;
    }

    /* Aligned space for kernel to store cmsghdr{}'s */
    cmsgspace = CMSG_SPACE(numfds * sizeof(int));
    cmsgp = malloc(cmsgspace);
    if(cmsgp == NULL) {
        error("malloc: %s\n", strerror(errno));
        return -2;
    }

    iov[0].iov_base     = databuf;
    iov[0].iov_len      = (size_t)datalen;
    msg.msg_iov         = iov;
    msg.msg_iovlen      = 1;
    msg.msg_name        = srcaddr;
    msg.msg_namelen     = (srcaddr) ? sizeof(struct sockaddr_un) : 0;
    msg.msg_control     = cmsgp;
    msg.msg_controllen  = cmsgspace;
    msg.msg_flags       = 0;

    errs = recvmsg(recvsock, &msg, 0);
    if(errs < 0) {
        error("recvmsg: %m\n");
        free(cmsgp);
        return -2;
    }
    debug("msg recvd: %d bytes, msg_flags: 0x%x, msg_cntllen: %ju\n", (int)errs,
            msg.msg_flags, (uintmax_t)msg.msg_controllen);
    if(msg.msg_controllen < sizeof(struct cmsghdr)) {
        error("Kernel didn't fill cmsg? msg.controllen: %ju\n",
                (uintmax_t)msg.msg_controllen);
        free(cmsgp);
        return -2;
    }

    /* Search through the cmsg's received */
    /* In case of no descriptors, the databuf should contain the errnum */
    tmp_cmsgp = CMSG_FIRSTHDR(&msg);
    for(; tmp_cmsgp != NULL; tmp_cmsgp = CMSG_NXTHDR(&msg, tmp_cmsgp)) {
        debug("Processing cmsg: len: %ju, level: %d, type: %d\n",
             (uintmax_t)tmp_cmsgp->cmsg_len, tmp_cmsgp->cmsg_level,
                tmp_cmsgp->cmsg_type);
        /* If it's the level and type for descriptors */
        if(tmp_cmsgp->cmsg_level == SOL_SOCKET &&
                tmp_cmsgp->cmsg_type == SCM_RIGHTS) {
            /* If it's the right length */
            if(tmp_cmsgp->cmsg_len == CMSG_LEN(numfds * sizeof(int))) {
                /* Grab the address of the first descriptor */
                int *data = (int *) CMSG_DATA(tmp_cmsgp);
                int i = 0;
                /* Store all of the passed descriptors into fdbuf */
                for(; i < numfds; i++, data++) {
                    fdbuf[i] = *data;
                }
            }
        }
    }

    free(cmsgp);
    return (int)errs;
}

/**
* Creates (and binds) the socket to send a request to the server.
* Returns: the socket, or -2 on error
*/
int socket_bind_fdd(char *tmpname) {
    int fddsock, tmpfile, err;
    struct sockaddr_un myaddr;

    /* Create socket to talk to fdd */
    fddsock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(fddsock < 0) {
        error("socket: %s\n", strerror(errno));
        return -2;
    }

    tmpfile = mkstemp(tmpname);
    if(tmpfile < 0) {
        error("mkstemp: %s\n", strerror(errno));
        close(fddsock);
        return -2;
    }
    /* We just use mkstemp to get a filename, so close the file. Dumb right? */
    close(tmpfile);
    unlink(tmpname);
    /* Fill out address to bind to */
    myaddr.sun_family = AF_LOCAL;
    /* Just in case we make tmpname longer than sun_path, still want '\0' */
    strncpy(myaddr.sun_path, tmpname, sizeof(myaddr.sun_path) - 1);

    err = bind(fddsock, (struct sockaddr*)&myaddr, sizeof(myaddr));
    if(err < 0) {
        error("bind: %s\n", strerror(errno));
        close(fddsock);
        return -2;
    }

    return fddsock;
}

/**
*  Acts like socket(2).
*  Sends request to the fd daemon to create a socket.
*  Blocks until receiving a response.
*
*  Returns: the socket descriptor, or
*           -1 if the user's socket() call failed
*           -2 if socketfd() failed for an internal reason
*           In either case errno is set appropriately.
*/
int socketfd(int domain, int type, int protocol) {
    struct fdreq req;
    int fddsock, usersock = -2;
    int n;
    struct sockaddr_un fddaddr = { /* fill out fdd's location */
            .sun_family = AF_LOCAL,
            .sun_path = FDD_SOCK_PATH
    };
    struct fdresp resp;
    char tmpname[] = CLI_SOCK_PATH;

    /* Create socket to talk to fdd */
    fddsock = socket_bind_fdd(tmpname);
    if(fddsock < 0) {
        error("socket_bind_fdd: Failed to create request socket.\n");
        return -2;
    }

    /* Fill out the request */
    req.fdcode = FDCODE_SOCKET;
    req.numfds = 1;
    req.fdreq_domain = domain;
    req.fdreq_type = type;
    req.fdreq_protocol = protocol;
    /* Send the fd request */
    n = (int) sendto(fddsock, &req, FDREQ_LEN(&req), 0,
            (struct sockaddr*)&fddaddr, sizeof(fddaddr));
    if(n < 0) {
        error("sendto: %s\n", strerror(errno));
        usersock = -2;
        goto cleanup;
    }
    /* Receive the descriptor */
    /* TODO: verify src from recv_fds */
    n = recv_fds(fddsock, NULL, &resp, sizeof(resp), &usersock, 1);
    debug("recv_fds: returned: %d\n", n);
    if(n < 0) {
        error("recv_fds failed from from previous errors.\n");
        goto cleanup;
    } else if(n != sizeof(resp)) {
        error("recv_fds didn't receive sizeof(struct fdreq).\n");
        usersock = -2;
        goto cleanup;
    }

    /* Now check fdd's response, non 0 if server's socket call failed */
    if(resp.errnum != 0) {
        errno = resp.errnum;
        usersock = -1;
    }

cleanup:
    close(fddsock);
    unlink(tmpname);
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
