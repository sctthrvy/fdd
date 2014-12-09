#include "fdd.h"
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> /* recvmsg(2) */
#include <sys/socket.h> /* recvmsg(2) */


/**
* Uses recvmsg(2) to receive descriptors.
*
* TODO: design
*/
int recv_fds(int sock , struct sockaddr_in *srcaddr, socklen_t slen, char *buf, size_t buflen) {
    ssize_t errs;
    struct msghdr msg;
    struct iovec iov;
    struct cmsghdr *cmsgp = malloc(100);
    if(cmsgp == NULL) {
        _ERROR("%s: %m\n", "malloc()");
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
        _ERROR("%s: %m\n", "recvmsg()");
        free(cmsgp);
        return -1;
    }
    if(msg.msg_controllen < sizeof(struct cmsghdr)) {
        _ERROR("%s", "Kernel didn't fill in control data, IP_RECVORIGDSTADDR didn't work?\n");
        free(cmsgp);
        return -1;
    }

    free(cmsgp);
    return (int)errs;
}
