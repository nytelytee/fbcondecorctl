#ifndef __FBCONDECORCTL_IMAGE_H
#define __FBCONDECORCTL_IMAGE_H

#include <stdio.h>
#include <stdbool.h>
#include "types.h"

u8* read_png_as_framebuffer(FILE* fp);
int write_framebuffer_as_png(FILE* fp, u8 *data, bool swap_dimensions);

#endif
