#include "fd.h"
#include "fdcommon.h"
#include "debug.h"


/**
* Creates (and binds) the socket to send a request to the server.
* Returns: the socket, or -1 on error
*/
static int socket_bind_fdd(char *tmpname) {
    int fddsock, tmpfile, err;
    struct sockaddr_un myaddr;

    /* Create socket to talk to fdd */
    fddsock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(fddsock < 0) {
        error("socket: %s\n", strerror(errno));
        return -1;
    }

    tmpfile = mkstemp(tmpname);
    if(tmpfile < 0) {
        error("mkstemp: %s\n", strerror(errno));
        close(fddsock);
        return -1;
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
        return -1;
    }

    return fddsock;
}

/**
* Uses sendto(2) to send the fdreq.
* @fddsock -- socket to send on
* @fdreqp -- request to send
*
* Returns: return value of sendto(2)
*/
static int send_req(int fddsock, struct fdreq *fdreqp) {
    int n;
    struct sockaddr_un fddaddr = { /* fill out fdd's location */
            .sun_family = AF_LOCAL,
            .sun_path = FDD_SOCK_PATH
    };

    n = (int)sendto(fddsock, fdreqp, FDREQ_LEN(fdreqp), 0,
            (struct sockaddr*)&fddaddr, sizeof(fddaddr));
    if(n < 0) {
        error("sendto: %s\n", strerror(errno));
    }
    return n;
}

/**
* Uses recvmsg(2) to receive descriptors.
* Data is stored into databuf, the file descriptors are stored into fdbuf.
* srcaddr can be NULL if you are not interested in the source address.
*
* Returns: number of data bytes received, or
*          -1 on error
*/
static int recv_resp(int recvsock, struct sockaddr_un *srcaddr,
        struct fdresp *resp, int *fdbuf, int numfds) {

    ssize_t errs;
    struct msghdr msg;
    struct iovec iov[1];
    struct cmsghdr *cmsgp, *tmp_cmsgp;
    size_t cmsgspace;

    if(resp <= 0 || numfds <= 0  || !fdbuf) {
        error("numfds=%d <= 0 || !(resp=%p) || !(fdbuf=%p)", numfds,
                resp, (void*)fdbuf);
        errno = EINVAL;
        return -1;
    }

    /* Aligned space for kernel to store cmsghdr{}'s */
    cmsgspace = CMSG_SPACE(numfds * sizeof(int));
    cmsgp = malloc(cmsgspace);
    if(cmsgp == NULL) {
        error("malloc: %s\n", strerror(errno));
        return -1;
    }

    iov[0].iov_base     = resp;
    iov[0].iov_len      = sizeof(struct fdresp);
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
        return -1;
    }
    debug("msg recvd: %d bytes, msg_flags: 0x%x, msg_cntllen: %ju\n", (int)errs,
            msg.msg_flags, (uintmax_t)msg.msg_controllen);
    if(resp->errnum == 0 && msg.msg_controllen < sizeof(struct cmsghdr)) {
        error("Didn't get any cmsgs: msg.controllen=%ju\n",
                (uintmax_t)msg.msg_controllen);
        free(cmsgp);
        return -1;
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


static int process_fdreq(struct fdreq *fdreqp, int *fdbuf, int numfds) {
    int fddsock, rtnval = -1;
    int n, set_errno = 0;
    struct fdresp resp;
    char tmpname[] = CLI_SOCK_PATH;

    /* Create socket to talk to fdd */
    fddsock = socket_bind_fdd(tmpname);
    if(fddsock < 0) {
        error("socket_bind_fdd: Failed to create request socket.\n");
        return -1;
    }

    /* Send the fd request to the server */
    n = send_req(fddsock, fdreqp);
    if(n < 0) {
        error("send_req: failed\n");
        goto cleanup;
    }
    /* Receive the descriptor */
    /* TODO: verify src from recv_resp */
    n = recv_resp(fddsock, NULL, &resp, fdbuf, numfds);
    debug("recv_resp: returned: %d data bytes recv'd\n", n);
    if(n < 0) {
        error("recv_resp failed from from previous errors.\n");
        goto cleanup;
    } else if(n != sizeof(resp)) {
        error("recv_resp didn't receive sizeof(struct fdreq).\n");
        goto cleanup;
    }

    /* Now check fdd's response, non 0 if server's socket call failed */
    if(resp.errnum != 0) {
        set_errno = 1;
        rtnval = -1;
    } else {
        rtnval = 0;
    }

    cleanup:
    close(fddsock);
    unlink(tmpname);
    if(set_errno)
        errno = resp.errnum; /* set errno correctly */
    return rtnval;
}

/**
*  Acts like socket(2).
*  Sends request to the fd daemon to create a socket.
*  Blocks until receiving a response.
*
*  Returns: the socket descriptor, or
*           -1 on failure with errno set
*/
int socketfd(int domain, int type, int protocol) {
    int err, usersock = -1;
    struct fdreq req;

    /* Fill out the request */
    req.fdcode = FDCODE_SOCKET;
    req.fdreq_domain = domain;
    req.fdreq_type = type;
    req.fdreq_protocol = protocol;

    err = process_fdreq(&req, &usersock, 1);
    if(err)
        return err;
    return usersock;
}


/* Acts like socketpair(2) */
int socketpairfd(int domain, int type, int protocol, int sv[2]) {
    struct fdreq req;

    /* Fill out the request */
    req.fdcode = FDCODE_SOCKETPAIR;
    req.fdreq_domain = domain;
    req.fdreq_type = type;
    req.fdreq_protocol = protocol;

    return process_fdreq(&req, sv, 2);
}

/* Acts like open(2) */
int openfd(const char *pathname, int flags) {
    int err, usersock;
    struct fdreq req;

    /* Fill out the request */
    req.fdcode = FDCODE_OPEN;
    req.fdreq_flags = flags;
    strncpy(req.fdreq_path, pathname, sizeof(req.fdreq_path));

    err = process_fdreq(&req, &usersock, 1);
    if(err)
        return err;
    return usersock;
}

/* Acts like pipe(2) */
int pipefd(int pipefd[2]) {
    struct fdreq req;

    /* Fill out the request */
    req.fdcode = FDCODE_PIPE;

    return process_fdreq(&req, pipefd, 2);
}
