/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>

#include "arg.h"
#include "slstatus.h"
#include "util.h"

struct arg {
	const char *(*func)();
	const char *fmt;
	const char *args;
    int update_once;
};

char buf[1024];
static volatile sig_atomic_t done;
static Display *dpy;

#include "config.h"

void update_once(size_t *initial_len, char status[MAXLEN], size_t status_len);

size_t initial_len = 0;
char status[MAXLEN];

static void
terminate(const int signo)
{
    update_once(&initial_len, status, sizeof(status));
	if (signo != SIGUSR1)
		done = 1;
}

static void
difftimespec(struct timespec *res, struct timespec *a, struct timespec *b)
{
	res->tv_sec = a->tv_sec - b->tv_sec - (a->tv_nsec < b->tv_nsec);
	res->tv_nsec = a->tv_nsec - b->tv_nsec +
	               (a->tv_nsec < b->tv_nsec) * 1E9;
}

static void
usage(void)
{
	die("usage: %s [-s] [-1]", argv0);
}

// making sflag global for the
// update_once function to have access to it
int sflag;

int
main(int argc, char *argv[])
{
	struct sigaction act;
	struct timespec start, current, diff, intspec, wait;
	size_t i, len;
	int ret;
	const char *res;

	sflag = 0;
	ARGBEGIN {
		case '1':
			done = 1;
			/* fallthrough */
		case 's':
			sflag = 1;
			break;
		default:
			usage();
	} ARGEND

	if (argc) {
		usage();
	}

	memset(&act, 0, sizeof(act));
	act.sa_handler = terminate;
	sigaction(SIGINT,  &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	act.sa_flags |= SA_RESTART;
	sigaction(SIGUSR1, &act, NULL);

	if (!sflag && !(dpy = XOpenDisplay(NULL))) {
		die("XOpenDisplay: Failed to open display");
	}

	status[0] = '\0';
    update_once(&initial_len, status, sizeof(status));

	do {
        
		if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
			die("clock_gettime:");
		}

        len = initial_len;
		for (i = 0; i < LEN(args); i++) {
            if (!args[i].update_once){
                if (!(res = args[i].func(args[i].args))) {
                    res = unknown_str;
                }
                if ((ret = esnprintf(status + len, sizeof(status) - len,
                                    args[i].fmt, res)) < 0) {
                    break;
                }
                len += ret;
            }
		}

		if (sflag) {
			puts(status);
			fflush(stdout);
			if (ferror(stdout))
				die("puts:");
		} else {
			if (XStoreName(dpy, DefaultRootWindow(dpy), status)
                            < 0) {
				die("XStoreName: Allocation failed");
			}
			XFlush(dpy);
		}

		if (!done) {
			if (clock_gettime(CLOCK_MONOTONIC, &current) < 0) {
				die("clock_gettime:");
			}
			difftimespec(&diff, &current, &start);

			intspec.tv_sec = interval / 1000;
			intspec.tv_nsec = (interval % 1000) * 1E6;
			difftimespec(&wait, &intspec, &diff);

			if (wait.tv_sec >= 0) {
				if (nanosleep(&wait, NULL) < 0 &&
				    errno != EINTR) {
					die("nanosleep:");
				}
			}
		}
	} while (!done);

	if (!sflag) {
		XStoreName(dpy, DefaultRootWindow(dpy), NULL);
		if (XCloseDisplay(dpy) < 0) {
			die("XCloseDisplay: Failed to close display");
		}
	}

	return 0;
}

void
update_once(size_t *len, char status[MAXLEN], size_t status_len) {

    *len=0;

	size_t i;
	int ret;
	const char *res;

    status[0] = '\0';
    for (i = 0; i < LEN(args); i++) { 
        if (args[i].update_once){
            if (!(res = args[i].func(args[i].args))) {
                res = unknown_str;
            }
            if ((ret = esnprintf(status + *len, status_len - *len,
                                args[i].fmt, res)) < 0) {
                break;
            }
            *len += ret;
        }
    }
}
