#include "fdd.h"
#include "fdcommon.h"

static int usock; /* UNIX socket for communication with libfd */

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


int main(int argc, char *argv[]) {

    /* Set SIGINT handler to socket file */
    set_sig_cleanup();
    usock = initusock();

    return 0;
}

int initusock(void) {
    int sock;
    struct sockaddr_un uaddr;

    /* Create UNIX socket */
    sock = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(sock < 0) {
        error("Failed to create UNIX socket: %s\n", strerror(errno));
        return 0;
    }
    /* Bind to upath */
    uaddr.sun_family = AF_UNIX;
    strncpy(uaddr.sun_path, FDD_SOCK_PATH, sizeof(uaddr.sun_path) - 1);
    /* unlink the file, in case it exists already */
    unlink(FDD_SOCK_PATH);
    if(bind(sock, (struct sockaddr *)&uaddr, sizeof(uaddr)) < 0) {
        error("Failed to bind to %s: %s\n", FDD_SOCK_PATH, strerror(errno));
        close(sock);
        return 0;
    }
    /* fchmod the file to world writable (722) so any process can use ODR */
    if(fchmod(sock, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
            < 0) {
        error("chmod failed: %s\n", strerror(errno));
        close(sock);
        return 0;
    }

    return sock;
}