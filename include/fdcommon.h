#ifndef _FDCOMMON_H
#define _FDCOMMON_H 1

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>     /* for FILENAME_MAX */
#include <unistd.h>
#include <string.h>     /* for strlen() */
#include <errno.h>
#include <inttypes.h>   /* for intmax_t */

#include <sys/types.h>  /* for send/recvmsg() */
#include <sys/socket.h> /* for send/recvmsg() */
#include <sys/un.h>     /* for UNIX sockets() */


#define FDD_SOCK_PATH "/tmp/fdd.socket"

/* Request message sent from libfd to fdd */
struct fdreq {
    int fdcode;
    int numfds;
    union {
        struct { /* socket(2)/socketpair(2) args */
            int domain;
            int type;
            int protocol;
        } sockreq;

        struct {
            int flags; /* flags for open(2) */
            char path[FILENAME_MAX];
        } openreq;
    } fdreq_un;

/* Correct way to access a fdreq{} if it is for socket(2) */
#define fdreq_domain            fdreq_un.sockreq.domain
#define fdreq_type              fdreq_un.sockreq.type
#define fdreq_protocol          fdreq_un.sockreq.protocol

/* Correct way to access a fdreq{} if it is for open(2) */
#define fdreq_flags             fdreq_un.openreq.flags
#define fdreq_path              fdreq_un.openreq.path
};

#define FDCODE_SOCKET           1
#define FDCODE_SOCKETPAIR       2
#define FDCODE_OPEN             4
#define FDCODE_PIPE             8
#define FDCODE_PIPE2            16

/* Is the fdreq for a socket() call? */
#define IS_SOCKREQ(fdreqp)      ((fdreqp)->fdcode & FDCODE_SOCKET)
/* Is the fdreq for a socketpair() call? */
#define IS_SOCKPAIRREQ(fdreqp)  ((fdreqp)->fdcode & FDCODE_SOCKETPAIR)
/* Is the fdreq for an open() call? */
#define IS_OPENREQ(fdreqp)      ((fdreqp)->fdcode & FDCODE_OPEN)

#define FDREQ_LEN(fdreqp)       ((fdreqp)->fdcode & FDCODE_SOCKET) ? \
                                (5 * sizeof(int)) : \
                                (3 * sizeof(int) + strlen((fdreqp)->fdreq_path))

/**
* Response message sent from fdd to libfd.
* The actual descriptor(s) is(are) received in the cmsghdr's using recvmsg().
*/
struct fdresp {
    int errnum; /* A valid errno: 0 if success, non-zero if failure */
};

#endif /* _FDCOMMON_H */
