/*
 * Copyright (C) 2004-2007, Michal Januszewski <spock@gentoo.org>
 * Copyright (C) 2026-2026, NyteLyte <nytelyte@tuta.io>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <device.h>     // for fb_get_settings, fbd
#include <errno.h>      // for errno
#include <fcntl.h>      // for O_RDWR, open
#include <linux/fb.h>   // for fb_var_screeninfo, fb_bitfield, FBIOGET_FSCRE...
#include <stdbool.h>    // for false, true
#include <sys/ioctl.h>  // for ioctl
#include <types.h>      // for fb_data
#include <unistd.h>     // for close
#include <utility.h>    // for MSG_ERROR, iprint

struct fb_data fbd;

int fb_get_settings(void)
{
	int fb_fd = open("/dev/fb0", O_RDWR);

	if (fb_fd == -1) {
		iprint(MSG_ERROR, "Failed to open framebuffer device. (errno=%d)\n", errno);
		return 1;
	}

	if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbd.var) == -1) {
		iprint(MSG_ERROR, "Failed to get fb_var info. (errno=%d)\n", errno);
		return 2;
	}

	if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbd.fix) == -1) {
		iprint(MSG_ERROR, "Failed to get fb_fix info. (errno=%d)\n", errno);
		return 3;
	}

	if (fbd.var.bits_per_pixel != 32) {
		iprint(MSG_ERROR, "Unsupported depth: %d bpp. Only 32 bpp is supported.\n", fbd.var.bits_per_pixel);
		return 4;
	}

	if (fbd.fix.visual != FB_VISUAL_TRUECOLOR) {
		iprint(MSG_ERROR, "Unsupported visual mode. Only TrueColor is supported.\n");
		return 5;
	}

	fbd.bytespp = (fbd.var.bits_per_pixel + 7) >> 3;

	if (fbd.var.blue.length != 8 || fbd.var.green.length != 8 || fbd.var.red.length != 8) {
		fbd.opt = false;
		fbd.rlen = fbd.var.red.length;
		fbd.glen = fbd.var.green.length;
		fbd.blen = fbd.var.blue.length;
	} else {
		fbd.opt = true;
		fbd.ro = (fbd.var.red.offset >> 3);
		fbd.go = (fbd.var.green.offset >> 3);
		fbd.bo = (fbd.var.blue.offset >> 3);
	}

	close(fb_fd);

	return 0;
}
