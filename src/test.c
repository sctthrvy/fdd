#include <stdlib.h> /* exit */
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h> /* strerror */
#include <errno.h>
#include <debug.h>

#include "fd.h" /* socketfd */

void testsocketfd(void) {
    int sock, err;

    sock = socketfd(PF_PACKET, SOCK_DGRAM, 0);
    if(sock < 0) {
        if(sock == -1)
            error("socketfd() socket(): %s\n", strerror(errno));
        else if(sock == -2)
            error("socketfd(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    err = close(sock);
    if(err) {
        error("close: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    success("Created socket with socketfd().\n");
}

void testopen(void) {
    char *filename = "/tmp/test_openfd";
    int fd, err;

    fd = openfd(filename, O_RDWR);
    if(fd < 0) {
        if(fd == -1)
            error("openfd() open(): %s\n", strerror(errno));
        else if(fd == -2)
            error("openfd(): %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    err = (int) write(fd, "Hello World", sizeof("Hello World"));
    if(err < 0) {
        error("write: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    err = close(fd);
    if(err) {
        error("close: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    success("Opened file with openfd().\n");
}

int main(void) {

    testsocketfd();

    testopen();

    return 0;
}