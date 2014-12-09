#include "fddcommon.h"


int main(int argc, char *argv[]) {

    /* Set SIGINT handler to socket file */
    set_sig_cleanup();

    /* Create UNIX socket */
    if((unixsock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        error("socket failed: %s\n", strerror(errno));
        goto CLOSE_RAW;
    }
    /* Bind to well known file */
    unaddr.sun_family = AF_UNIX;
    strncpy(unaddr.sun_path, ODR_PATH, sizeof(unaddr.sun_path) - 1);
    /* unlink the file */
    unlink(ODR_PATH);
    if(bind(unixsock, (struct sockaddr*)&unaddr, sizeof(unaddr)) < 0) {
        error("bind failed: %s\n", strerror(errno));
        goto CLOSE_UNIX;
    }

    /* chmod the file to world writable (722) so any process can use ODR  */
    if(chmod(ODR_PATH, S_IRUSR| S_IWUSR| S_IXUSR | S_IWGRP | S_IWOTH) < 0) {
        error("chmod failed: %s\n", strerror(errno));
        goto CLOSE_UNIX;
    }




    return 0;
}
