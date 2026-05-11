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

#define FBIO_CONDECOR _IOWR('F', 0x19, struct fbcondecor_command)

// Flag to send to the ioctl
#define FBCONDECOR_COMMAND_FLAG_QUERY 0
#define FBCONDECOR_COMMAND_FLAG_GET_GEOMETRY 1
#define FBCONDECOR_COMMAND_FLAG_GET_IMAGE 2
#define FBCONDECOR_COMMAND_FLAG_SET_GEOMETRY 4
#define FBCONDECOR_COMMAND_FLAG_SET_IMAGE 8

// Flags for the response from the ioctl
#define FBCONDECOR_STATE_FLAG_GEOMETRY 1
#define FBCONDECOR_STATE_FLAG_IMAGE 2
#define FBCONDECOR_STATE_FLAG_VC_ROTATION_LOW 4 
#define FBCONDECOR_STATE_FLAG_VC_ROTATION_HIGH 8
      
// Utilities for interfacing with the state flags
#define FBCONDECOR_STATE_HAS_GEOMETRY(state) (\
      (state) & FBCONDECOR_STATE_FLAG_GEOMETRY\
)

#define FBCONDECOR_STATE_HAS_IMAGE(state) (\
      (state) & FBCONDECOR_STATE_FLAG_IMAGE\
)

#define FBCONDECOR_STATE_HAS_SWAPPED_DIMENSIONS(state) (\
      (state) & FBCONDECOR_STATE_FLAG_VC_ROTATION_LOW\
)

#define FBCONDECOR_ROTATION_SHIFT 2

#define FBCONDECOR_STATE_GET_ROTATION(state) (\
      (\
      	(state) & (\
      		FBCONDECOR_STATE_FLAG_VC_ROTATION_LOW |\
      		FBCONDECOR_STATE_FLAG_VC_ROTATION_HIGH\
      	)\
      ) >> FBCONDECOR_ROTATION_SHIFT\
)

#endif
