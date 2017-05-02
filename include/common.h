#ifndef _COMMON_H
#define _COMMON_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#define MAXLINE (1 << 12)

#define FILEMODE    (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DIRMODE     (FILEMODE | S_IXUSR | S_IXGRP | S_IXOTH)

void err_msg(const char *, ...);
void err_dump(const char *, ...);
void err_exit(int error, const char*, ...);

#endif
