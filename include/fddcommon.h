#ifndef FDD_INTERNAL_H
#define FDD_INTERNAL_H 1
#include "debug.h"

#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>  /* send/recvmsg */
#include <sys/socket.h> /* send/recvmsg */
#include <sys/un.h>     /* UNIX sockets */


#define FDD_SOCK_PATH "/var/run/fdd.socket"

#endif /* FDD_INTERNAL_H */