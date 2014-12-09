#include "fdcommon.h"
#include <sys/stat.h>   /* chmod(2) */
#include <signal.h>     /* sigaction(2) */

/* Function prototypes */
static void runfdd(int usock);
static int initusock(void);
static int validmsg(ssize_t nrecv, struct msghdr *msg);
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
    ssize_t nrecv;
    int flags = 0;
    struct msghdr msg;

    while(1) {
        nrecv = recvmsg(usock, &msg, flags);
        if(nrecv < 0) {
            /* Possible errors EIO, ENBUFS, and ENOMEM (all really bad) */
            error("recvmsg failed: %s\n", strerror(errno));
            return;
        }
        /* Validate the message */
        if(!validmsg(nrecv, &msg)) {
            debug("Skipping invalid message.\n");
            continue;
        }
        /* Process */

    }
}

/*
 * Creates a UNIX datagram socket for communication with libfd.
 */
static int initusock(void) {
    int sock, err;
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
    /* unlink the file, in case it exists already */
    unlink(FDD_SOCK_PATH);
    /* Bind to upath */
    err = bind(sock, (struct sockaddr *)&uaddr, sizeof(uaddr));
    if(err) {
        error("Failed to bind to %s: %s\n", FDD_SOCK_PATH, strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }
    /* fchmod the file to 766 so any process can use ODR */
    err = fchmod(sock, S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if(err) {
        error("chmod failed: %s\n", strerror(errno));
        close(sock);
        exit(EXIT_FAILURE);
    }
    return sock;
}

static int validmsg(ssize_t nrecv, struct msghdr *msg) {
    int valid = 0;
    if(nrecv < 0xFDD) {
        /* Message too short */
        debug("Short message: recvmsg returned %lld\n", (long long)nrecv);
    } else {
        /* Could be valid */
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
