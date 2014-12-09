#ifndef _FDCOMMON_H
#define _FDCOMMON_H 1
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>  /* send/recvmsg */
#include <sys/socket.h> /* send/recvmsg */
#include <sys/un.h>     /* UNIX sockets */


#define FDD_SOCK_PATH "/tmp/fdd.socket"

#endif /* _FDCOMMON_H */