#include <common.h>
#include <errno.h>
#include <stdarg.h>

static void error_doit(int flags, int error, const char *fmt, va_list arg)
{
    char buf[MAXLINE];

    vsnprintf(buf, MAXLINE -1, fmt, arg);
    if (flags)
        snprintf(buf + strlen(buf), MAXLINE - strlen(buf) - 1, ": %s", strerror(error));

    strcat(buf, "\n");
    fflush(stdout);
    fputs(buf, stderr);
    fflush(NULL);
}

void err_exit(int error, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    error_doit(1, error, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

/*
 * Nonfatal error unrelated to a system call
 * Print a message and return
 */
void err_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    error_doit(0, 0, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void err_dump(const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    error_doit(1, errno, fmt, arg);
    va_end(arg);
    abort();    /* dump core and terminate */
    exit(EXIT_FAILURE);
}


