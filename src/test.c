#include <stdlib.h> /* exit */
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h> /* strerror */
#include <errno.h>
#include <debug.h>

#include "fd.h" /* socketfd */

int main(void) {
    int sock;

    sock = socketfd(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        if(sock == -1)
            error("socketfd() socket(): %s\n", strerror(errno));
        else if(sock == -2)
            error("socketfd(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    success("Created socket with socketfd().\n");
    close(sock);
    exit(EXIT_SUCCESS);
}