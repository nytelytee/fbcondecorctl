#ifndef _UAPI_LINUX_FBCONDECOR_H
#define _UAPI_LINUX_FBCONDECOR_H

#include <linux/types.h>
#include <linux/fb.h>

// Geometry
struct fbcondecor_geometry {
	__u16 offset_x, offset_y; /* Top left corner  */
	__u16 width, height;      /* Width and height of the text field */
};

// The command that is sent to the ioctl
// get/set_image is a raw buffer of size 4*fb_width*fb_height,
// when setting, the image is in the framebuffer's native format
struct fbcondecor_command
{
	__u16 flag;         /* The type of the pointer for a command */
	__u16 vc;           /* The console to target with the ioctl */
	__u32 pad;
	__u64 get_geometry; /* Pointer to buffer, or NULL if not getting */
	__u64 set_geometry; /* Pointer to geometry, or NULL if turning off */
	__u64 get_image;    /* Pointer to buffer, or NULL if not getting */
	__u64 set_image;    /* Pointer to image, or NULL if turning off */
};

// Flag to send to the ioctl
#define FBCONDECOR_FLAG_QUERY 0
#define FBCONDECOR_FLAG_GET_GEOMETRY 1
#define FBCONDECOR_FLAG_GET_IMAGE 2
#define FBCONDECOR_FLAG_SET_GEOMETRY 4
#define FBCONDECOR_FLAG_SET_IMAGE 8

// The response from the ioctl, can also be treated as a flag
#define FBCONDECOR_STATE_OFF 0
#define FBCONDECOR_STATE_GEOMETRY 1
#define FBCONDECOR_STATE_IMAGE 2
#define FBCONDECOR_STATE_FULL 3

#define FBIO_CONDECOR _IOWR('F', 0x19, struct fbcondecor_command)

#endif
