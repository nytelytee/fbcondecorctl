#ifndef __FBCONDECORCTL_PARSE_H
#define __FBCONDECORCTL_PARSE_H

#include <stdbool.h>

long parse_int(const char *str, bool *ok);

long parse_tty(const char *str, int command);

long parse_slot(const char *str, int command, int min);

void require_args(int i, int argc, int n, int command);

#define OP_NONE 0
#define OP_SET 1
#define OP_GET 2
#define OP_SWAP 3

struct command_ops parse_op(const char* cmd, int command);

#endif
