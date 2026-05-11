#ifndef __FBCONDECORCTL_UTILITY_H
#define __FBCONDECORCTL_UTILITY_H

#include <stdio.h>

#define PACKAGE "fbcondecorctl"
#define VERSION "1.1"

#define PATH_DEV	"/dev"
#define PATH_PROC	"/proc"
#define PATH_SYS	"/sys"
#define FBSPLASH_DIR "/usr/lib/splash"
#define FBCON_DECOR_DEV	PATH_DEV "/fbcondecor"

#define FBCON_DECOR_THEME_LEN 128

#define FBCON_DECOR_IO_ORIG_KERNEL 0
#define FBCON_DECOR_IO_ORIG_USER 1

#define MSG_CRITICAL	0
#define MSG_ERROR		1
#define MSG_WARN		2
#define MSG_INFO		3

// TODO: REMOVE
#define min(a,b)		((a) < (b) ? (a) : (b))
#define max(a,b)		((a) > (b) ? (a) : (b))

#define iprint(type, ...) do {				\
	if (type <= MSG_INFO) {					\
		fprintf(stderr, ## __VA_ARGS__);				\
	}											\
} while (0);

#define CLAMP(x)		((x) > 255 ? 255 : (x))
#define DEBUG(...)

#endif /* __H_FBCONDECORCTL_UTILITY */
