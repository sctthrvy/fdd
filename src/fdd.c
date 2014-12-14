/* For credentials */
#define _GNU_SOURCE
#include "fdcommon.h"
#include "debug.h"
#include <sys/stat.h>   /* chmod(2)     */
#include <fcntl.h>      /* open(2)      */
#include <signal.h>     /* sigaction(2) */

/* Function prototypes */
static void runfdd(int usock);
ssize_t recvfdreq(int usock, struct fdreq* req, struct ucred *cred, struct
        sockaddr_un *src);
ssize_t sendfdresp(int usock, struct fdresp *resp, int *fds, int numfds,
        struct sockaddr_un *src);
int handle_fdreq(ssize_t nrecv, struct fdreq *req, int fds[2], int *numfds);
static int initusock(void);
static int validmsg(ssize_t nrecv, struct fdreq *req);
static void cleanup(int signum);
static void set_sig_cleanup(void);

int main(int argc, char *argv[]) {
    int usock;

    /* Set SIGINT handler to socket file */
    set_sig_cleanup();
    usock = initusock();

    /* Start the fd daemon */
    runfdd(usock);

    /* Cleanup if service fails */
    error("fdd service failed: %s\n", strerror(errno));
    if(close(usock)) {
        error("failed to close UNIX socket: %s\n", strerror(errno));
    }
    if(unlink(FDD_SOCK_PATH)) {
        error("Failed to remove %s: %s\n", FDD_SOCK_PATH, strerror(errno));
    }
    return EXIT_FAILURE;
}

/*
 * @usock A valid UNIX socket read to recveive messages
 */
static void runfdd(int usock) {
    ssize_t nrecv, nsent;
    struct sockaddr_un src;
    struct fdreq req;
    struct fdresp resp;
    struct ucred cred;
    int numfds, fds[2], i;

    debug("FDD waiting for requests\n");
    while(1) {
        nrecv = recvfdreq(usock, &req, &cred, &src);
        if(nrecv < 0) {
            /* Fatal error on recvmsg(2) */
            return;
        }
        /* Validate the message */
        if(!validmsg(nrecv, &req)) {
            debug("Skipping invalid message.\n");
            continue;
        }
        /* Process request */
        resp.errnum = handle_fdreq(nrecv, &req, fds, &numfds);

        /* Send reply */
        nsent = sendfdresp(usock, &resp, fds, numfds, &src);
        if(nsent < 0) {
            error("sendmsg failed: %s\n", strerror(errno));
        }
        /* Close the file descriptors */
        for(i = 0; i < numfds; ++i) {
            if(close(fds[i])) {
                warn("Failed to close fd: %s\n", strerror(errno));
            }
        }
    }
}

/*
 * @usock A valid UNIX socket read to recveive messages
 */
ssize_t recvfdreq(int usock, struct fdreq* req, struct ucred *cred, struct
        sockaddr_un *src) {
    ssize_t nrecv;
    struct msghdr msg;
    struct iovec iv;
    struct cmsghdr *cmsg, *tmp_cmsg;

    cmsg = malloc(CMSG_SPACE(sizeof(struct ucred)));
    if(cmsg == NULL) {
        error("malloc failed: %s\n", strerror(errno));
        return -1;
    }
    iv.iov_base        = req;
    iv.iov_len         = sizeof(struct fdreq);
    msg.msg_name       = src;
    msg.msg_namelen    = sizeof(struct sockaddr_un);
    msg.msg_iov        = &iv;
    msg.msg_iovlen     = 1;
    msg.msg_control    = cmsg;
    msg.msg_controllen = CMSG_SPACE(sizeof(struct ucred));
    msg.msg_flags      = 0;

    nrecv = recvmsg(usock, &msg, 0);
    if(nrecv < 0) {
        /* Possible errors EIO, ENBUFS, and ENOMEM (all really bad) */
        error("recvmsg failed: %s\n", strerror(errno));
    } else {
        tmp_cmsg = CMSG_FIRSTHDR(&msg);
        if(tmp_cmsg && tmp_cmsg->cmsg_level == SOL_SOCKET &&
                tmp_cmsg->cmsg_type == SCM_CREDENTIALS) {
            debug("Received credentials.\n");
            memcpy(cred, CMSG_DATA(tmp_cmsg), sizeof(struct ucred));
        } else {
            debug("No credentials received.\n");
        }
    }

    free(cmsg);
    return nrecv;
}

ssize_t sendfdresp(int usock, struct fdresp *resp, int *fds, int numfds,
        struct sockaddr_un *src) {
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    struct iovec iv;
    size_t datalen = numfds * sizeof(int);
    ssize_t nsent;
    cmsg = malloc(CMSG_SPACE(datalen));
    if(cmsg == NULL) {
        error("malloc failed: %s\n", strerror(errno));
        return -1;
    }
    /* Setup ancillary data with file descriptors */
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(datalen);
    memcpy(CMSG_DATA(cmsg), fds, datalen);
    /* Setup normal data to send */
    iv.iov_base = resp;
    iv.iov_len  = sizeof(struct fdresp);
    /* Setup the actual message */
    msg.msg_name = src;
    msg.msg_namelen = SUN_LEN(src);
    msg.msg_iov   = &iv;
    msg.msg_iovlen = 1;
    msg.msg_control = cmsg;
    msg.msg_controllen = cmsg->cmsg_len;
    /* Send back the client */
    nsent = sendmsg(usock, &msg, 0);
    return nsent;
}

/**
*
* @nrecv   Number of bytes in the received req
* @req     Pointer to a valid fdreq struct
* @fds     Array to store the created file descriptors
* @numfds  Result argument, stores the number of successfully created fd's
*
* returns  The error number to return to the client
*/
int handle_fdreq(ssize_t nrecv, struct fdreq *req, int fds[2], int *numfds) {
    int err = 0;

    *numfds = 0;
    switch(req->fdcode) {
        case FDCODE_SOCKET:
            fds[0] = socket(req->fdreq_domain, req->fdreq_type,
                            req->fdreq_protocol);
            if(fds[0] < 0) {
                err = errno;
                warn("failed to create socket: %s\n", strerror(errno));
            } else {
                *numfds = 1;
            }
            break;
        case FDCODE_SOCKETPAIR:
            err = socketpair(req->fdreq_domain, req->fdreq_type,
                             req->fdreq_protocol, fds);
            if(err) {
                err = errno;
                warn("failed to create socketpair: %s\n", strerror(errno));
            } else {
                *numfds = 2;
            }
            break;
        case FDCODE_OPEN:
            /* Null terminate the received pathname */
            ((char *)req)[nrecv] = '\0';
            info("req: open(%s, 0x%X, %o)\n", req->fdreq_path, req->fdreq_flags, req->fdreq_mode);
            fds[0] = open(req->fdreq_path, req->fdreq_flags, req->fdreq_mode);
            if(fds[0] < 0) {
                err = errno;
                warn("failed to open %s: %s\n", req->fdreq_path,
                     strerror(errno));
            } else {
                *numfds = 1;
            }
            break;
        case FDCODE_PIPE:
            err = pipe(fds);
            if(err) {
                err = errno;
                warn("failed to create pipe: %s\n", strerror(errno));
            } else {
                *numfds = 2;
            }
            break;
        case FDCODE_PIPE2:
            /* Not implemented */
            err = ENOSYS;
            break;
        default:
            /* Invalid request */
            err = EINVAL;
            info("Recveived unknown fdreq code: %d\n", req->fdcode);
    }
    return err;
}

/*
 * Creates a UNIX datagram socket for communication with libfd.
 */
static int initusock(void) {
    int sock, err, opt;
    struct sockaddr_un uaddr = {
            .sun_family = AF_UNIX,
            .sun_path   = FDD_SOCK_PATH
    };
    /* Create UNIX socket */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sock < 0) {
        error("Failed to create UNIX socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    debug("Created UNIX socket\n");
    /* unlink the file, in case it exists already */
    unlink(FDD_SOCK_PATH);
    /* Bind to upath */
    err = bind(sock, (struct sockaddr *)&uaddr, sizeof(uaddr));
    if(err) {
        error("Failed to bind to %s: %s\n", FDD_SOCK_PATH, strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }
    debug("Binded UNIX socket to %s\n", FDD_SOCK_PATH);
    /* Enable the receiving of credentials */
    opt = 1;
    err = setsockopt(sock, SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt));
    if(err) {
        error("Failed to enable receiving credentials: %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }
    debug("Enabled reception of credentials.\n");
    /* chmod the file to 766 so any process can use ODR */
    err = chmod(FDD_SOCK_PATH, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP |
                S_IROTH | S_IWOTH);
    if(err) {
        error("chmod failed: %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }
    return sock;
}

static int validmsg(ssize_t nrecv, struct fdreq *req) {
    int valid = 0;
    if(nrecv < FDREQ_MIN) {
        /* Message too short */
        debug("Short message: recvmsg returned %lld\n", (long long)nrecv);
    } else {
        /* Could be valid */
        valid = 1;
    }
    return valid;
}

static void cleanup(int signum) {
    /* remove the UNIX socket file */
    unlink(FDD_SOCK_PATH);
    /* Standard Linux exit code 128 + fatal error signal */
    _exit(128 + signum);
}

static void set_sig_cleanup(void) {
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &cleanup;

    /* Set the sigaction */
    if(sigaction(SIGINT, &sa, NULL)) {
        error("sigaction failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}
