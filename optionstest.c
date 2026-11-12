#define _XOPEN_SOURCE 600   /* for posix_openpt() */

#include <sys/ioctl.h>
#include <sys/wait.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"
#include "options.h"

/* wider than a short holds: the width used to be stored in one */
#define WIDECOLUMNS 40000

int test_wide_terminal_width(void);
int test_wide_terminal_listing(void);

int main(int argc, char **argv)
{
    myname = "optionstest";

    test_wide_terminal_width();
    test_wide_terminal_listing();
    return 0;
}

/*
 * Open a pseudoterminal that is "columns" wide.
 *
 * Returns the terminal's file descriptor and sets *controller to the other
 * end, or returns -1 where there are no pseudoterminals to be had.
 */
static int openterminal(unsigned short columns, int *controller)
{
    int fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (fd == -1 || grantpt(fd) != 0 || unlockpt(fd) != 0) {
        return -1;
    }
    char *name = ptsname(fd);
    if (!name) {
        return -1;
    }
    int terminal = open(name, O_RDWR | O_NOCTTY);
    if (terminal == -1) {
        return -1;
    }
    struct winsize ws;
    memset(&ws, 0, sizeof ws);
    ws.ws_row = 24;
    ws.ws_col = columns;
    assert(ioctl(terminal, TIOCSWINSZ, &ws) == 0);
    *controller = fd;
    return terminal;
}

int test_wide_terminal_width(void)
{
    errorf("\n");   /* prints the function name */
    int controller;
    int terminal = openterminal(WIDECOLUMNS, &controller);
    if (terminal == -1) {
        errorf("SKIPPED: no pseudoterminal\n");
        return 0;
    }

    fflush(stdout);
    int savedstdout = dup(STDOUT_FILENO);
    assert(savedstdout != -1);
    assert(dup2(terminal, STDOUT_FILENO) != -1);
    Options *options = newoptions();
    assert(dup2(savedstdout, STDOUT_FILENO) != -1);
    close(savedstdout);

    assert(options != NULL);
    assert(options->displaymode == DISPLAY_IN_COLUMNS);
    assert(options->screenwidth == WIDECOLUMNS);

    freeoptions(options);
    close(terminal);
    close(controller);
    return 0;
}

/* l aborted on such a terminal as soon as it had two names to lay out */
int test_wide_terminal_listing(void)
{
    errorf("\n");   /* prints the function name */
    int controller;
    int terminal = openterminal(WIDECOLUMNS, &controller);
    if (terminal == -1) {
        errorf("SKIPPED: no pseudoterminal\n");
        return 0;
    }

    fflush(stdout);
    pid_t pid = fork();
    assert(pid != -1);
    if (pid == 0) {
        if (dup2(terminal, STDOUT_FILENO) == -1) {
            _exit(126);
        }
        execl("./l", "l", "-d", ".", "..", (char *)NULL);
        _exit(127);
    }

    int status;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);

    /* the terminal is still open here, so what l wrote is still readable;
       both names fit on the one line, the first padded to the second's width */
    char output[64];
    ssize_t nread = read(controller, output, sizeof(output) - 1);
    assert(nread > 0);
    output[nread] = '\0';
    assert(strstr(output, ".   ..") != NULL);

    close(terminal);
    close(controller);
    return 0;
}

/* vim: set ts=4 sw=4 tw=0 et:*/
