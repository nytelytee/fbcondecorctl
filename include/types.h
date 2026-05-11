#ifndef __FBCONDECORCTL_TYPES_H
#define __FBCONDECORCTL_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <linux/fb.h>

typedef uint8_t	  u8;
typedef uint16_t	u16;
typedef uint32_t	u32;
typedef int8_t		s8;
typedef int16_t		s16;
typedef int32_t		s32;

#define SLOT_NOTHING 0
#define SLOT_GEOMETRY 1
#define SLOT_IMAGE 2
#define SLOT_IMAGE_SWAPPED 3
struct slot { int type; u8* data; };

#define OP_NONE 0
#define OP_SET 1
#define OP_GET 2
#define OP_SWAP 3
struct command_ops { int geometry_op; int image_op; };

struct fb_data {
	struct fb_var_screeninfo   var;
	struct fb_fix_screeninfo   fix;
	int bytespp;			    /* bytes per pixel */
	bool opt;				      /* can we use optimized 32bpp routine? */
	u8 ro, go, bo;			  /* red, green, blue offset */
	u8 rlen, glen, blen;  /* red, green, blue length */
};

#endif
