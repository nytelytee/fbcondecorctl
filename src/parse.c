/*
 * Copyright (C) 2026-2026, NyteLyte <nytelyte@tuta.io>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <errno.h>     // for errno
#include <linux/vt.h>  // for MAX_NR_CONSOLES
#include <parse.h>     // for parse_int, parse_op, parse_slot, parse_tty
#include <stdbool.h>   // for bool, true, false
#include <stdlib.h>    // for exit, strtol
#include <string.h>    // for strcmp
#include <types.h>     // for command_ops
#include <utility.h>   // for MSG_ERROR, iprint

long parse_int(const char *str, bool *ok) {
	char *end;
	errno = 0;
	long val = strtol(str, &end, 10);

	if (errno != 0 || end == str || *end != '\0') {
			if (ok) *ok = false;
			return 0;
	}

	if (ok) *ok = true;
	return val;
}

long parse_tty(const char *str, int command) {
    bool ok = true;
    long slot = parse_int(str, &ok);
    if (!ok || slot > MAX_NR_CONSOLES || slot < 0) {
        iprint(MSG_ERROR, "Invalid tty argument in command %d. Must be a number in the range [0, %d].\n", command, MAX_NR_CONSOLES);
        exit(1);
    }
    return slot;
}

long parse_slot(const char *str, int command, int min) {
    bool ok = true;
    long slot = parse_int(str, &ok);
    if (!ok || slot > 100 || slot < min) {
        iprint(MSG_ERROR, "Invalid slot argument in command %d. Must be a number in the range [%d, 100].\n", command, min);
        exit(1);
    }
    return slot;
}

void require_args(int i, int argc, int n, int command) {
    if (i + n >= argc) {
        iprint(MSG_ERROR, "Missing argument(s) in command %d.\n", command);
        exit(1);
    }
}

struct command_ops parse_op(const char* cmd, int command) {
	if (!strcmp(cmd, "set-geometry")) return (struct command_ops){ OP_SET, OP_NONE };
	if (!strcmp(cmd, "set-image")) return (struct command_ops){ OP_NONE, OP_SET };
	if (!strcmp(cmd, "get-geometry")) return (struct command_ops){ OP_GET, OP_NONE };
	if (!strcmp(cmd, "get-image")) return (struct command_ops){ OP_NONE, OP_GET };
	if (!strcmp(cmd, "swap-geometry")) return (struct command_ops){ OP_SWAP, OP_NONE };
	if (!strcmp(cmd, "swap-image")) return (struct command_ops){ OP_NONE, OP_SWAP };
	if (!strcmp(cmd, "get-geometry-get-image")) return (struct command_ops){ OP_GET, OP_GET };
	if (!strcmp(cmd, "get-geometry-set-image")) return (struct command_ops){ OP_GET, OP_SET };
	if (!strcmp(cmd, "get-geometry-swap-image")) return (struct command_ops){ OP_GET, OP_SWAP };
	if (!strcmp(cmd, "set-geometry-get-image")) return (struct command_ops){ OP_SET, OP_GET };
	if (!strcmp(cmd, "set-geometry-set-image")) return (struct command_ops){ OP_SET, OP_SET };
	if (!strcmp(cmd, "set-geometry-swap-image")) return (struct command_ops){ OP_SET, OP_SWAP };
	if (!strcmp(cmd, "swap-geometry-get-image")) return (struct command_ops){ OP_SWAP, OP_GET };
	if (!strcmp(cmd, "swap-geometry-set-image")) return (struct command_ops){ OP_SWAP, OP_SET };
	if (!strcmp(cmd, "swap-geometry-swap-image")) return (struct command_ops){ OP_SWAP, OP_SWAP };
	if (!strcmp(cmd, "set")) return (struct command_ops){ OP_SET, OP_SET };
	if (!strcmp(cmd, "get")) return (struct command_ops){ OP_GET, OP_GET };
	if (!strcmp(cmd, "swap")) return (struct command_ops){ OP_SWAP, OP_SWAP };
	// assumption: this is called in the last branch of the main loop
	// so we can safely exit if it fails to parse
	iprint(MSG_ERROR, "Unknown command %d.\n", command);
	exit(1);
} 
