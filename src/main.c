/*
 * Copyright (C) 2004-2007 Michal Januszewski <spock@gentoo.org>
 * Copyright (C) 2026-2026 NyteLyte <nytelyte@tuta.io>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <linux/types.h>       // for __u64, __u16, __u32
#include <device.h>            // for fbd, fb_get_settings
#include <errno.h>             // for errno
#include <fcntl.h>             // for O_RDWR, open
#include <image.h>             // for read_png_as_framebuffer, write_framebu...
#include <inttypes.h>          // for uintptr_t, PRIu16, SCNu16
#include <linux/fb.h>          // for fb_var_screeninfo
#include <linux/fbcondecor.h>  // for fbcondecor_geometry, FBCONDECOR_FLAG_G...
#include <linux/vt.h>          // for vt_stat, VT_GETSTATE
#include <parse.h>             // for OP_SWAP, parse_slot, OP_SET, require_args
#include <png.h>               // for png_sig_cmp
#include <stdbool.h>           // for bool, false, true
#include <stdio.h>             // for NULL, size_t, FILE, fdopen, fopen, EOF
#include <stdlib.h>            // for exit, malloc, free, realloc
#include <string.h>            // for strcmp, memcpy
#include <sys/ioctl.h>         // for ioctl
#include <types.h>             // for slot, command_ops, u8, SLOT_GEOMETRY
#include <unistd.h>            // for close
#include <utility.h>           // for MSG_ERROR, iprint, FBCON_DECOR_DEV

#define ptr_to_u64(ptr) ((__u64)(uintptr_t)(ptr))

int fd_fbcondecor = -1;

int fbcondecor_execute_raw_command(
		int vc,
		int flag,
		struct fbcondecor_geometry *get_geometry,
		const struct fbcondecor_geometry *set_geometry,
		u8 *get_image,
		const u8 *set_image
) {

	struct fbcondecor_command command = {
		.flag = (__u16) flag,
		.vc = (__u16) vc,
		.pad = (__u32) 0,
		.get_geometry = ptr_to_u64(get_geometry),
		.set_geometry = ptr_to_u64(set_geometry),
		.get_image = ptr_to_u64(get_image),
		.set_image = ptr_to_u64(set_image)
	};

	int result = ioctl(fd_fbcondecor, FBIO_CONDECOR, &command);
	if (result >= 0) return result;
	return -errno;
}

static void usage()
{
	printf(
PACKAGE "-" VERSION "\n"
"Usage: fbcondecorctl [options] (<command> [command_arguments]) ...\n"
"Commands (they get executed on the currently selected tty):\n"
"NOTE: the currently selected tty is not the tty you are currently viewing, but the tty to perform the action on.\n"
"  tty      <tty_number>                                              change the currently selected tty (default is the currently viewed tty, use 0 to use the currently viewed tty)\n"
"Slot manipulation commands:\n"
"A slot is essentially a variable that holds data, you reference a slot by a number; initially, it starts off containing nothing (which is valid input for both set-image and set-geometry)\n"
"The slot 0 is special and always contains nothing, you cannot write to it. A slot has to be between 0 and 100.\n"
"  set-slot <slot-to-set> <slot>                                      make one slot hold the same data as another\n"
"  read-geometry <slot> <filename>                                    read the contents of the file into a slot as geometry in the form <offset_x> <offset_y> <width> <height>\n"
"  read-image <slot> <filename>                                       read the contents of the file into a slot as a png image\n"
"  write-file <slot> <filename>                                       write the contents of a slot; this will either be an empty file, a geometry, or a png image\n"
"Console decoration manipulation commands:\n"
"  set-geometry <slot>                                                set the fbcondecor geometry on the tty from a slot; the slot has to contain a geometry or nothing\n"
"  set-image <slot>                                                   set the image in png format on the tty from a slot; the slot has to contain either an image or nothing\n"
"  get-geometry <slot>                                                get the currently selected tty's geometry into a slot\n"
"  get-image <slot>                                                   get the current selected tty's image into a slot\n"
"  swap-geometry <slot>                                               set the geometry and get the old one\n"
"  swap-image <slot>                                                  set the image and get the old one\n"
"Advanced console decoration manipulation commands (these operations can be done with a single ioctl, so they may be useful; it should be obvious what they do from the name):\n"
"  get-geometry-get-image <geometry-slot> <image-slot>\n"
"  get-geometry-set-image <geometry-slot> <image-slot>\n"
"  get-geometry-swap-image <geometry-slot> <image-slot>\n"
"  set-geometry-get-image <geometry-slot> <image-slot>\n"
"  set-geometry-set-image <geometry-slot> <image-slot>\n"
"  set-geometry-swap-image <geometry-slot> <image-slot>\n"
"  swap-geometry-get-image <geometry-slot> <image-slot>\n"
"  swap-geometry-set-image <geometry-slot> <image-slot>\n"
"  swap-geometry-swap-image <geometry-slot> <image-slot>\n"
"  set <geometry-slot> <image-slot>                               alias for set-geometry-set-image\n"
"  get <geometry-slot> <image-slot>                               alias for get-geometry-get-image\n"
"  swap <geometry-slot> <image-slot>                              alias for swap-geometry-swap-image\n"

"Options:\n"
"  -h, --help               show this help message\n"
);
}


struct slot slots[101] = {0};

void set_slot(int slot, int type, u8* data) {
	slots[slot].type = type;
	slots[slot].data = data;
}

void read_file(int slot, const char* filename) {
	unsigned char png_sig[8];
	FILE* fp = NULL;
	if (!strcmp(filename, "-")) fp = fdopen(0, "rb");
	else fp = fopen(filename, "rb");
	if (fp == NULL) exit(1);
	size_t bytes_read = fread(png_sig, 1, 8, fp);
	if (bytes_read == 0) {
		set_slot(slot, SLOT_NOTHING, NULL);
		return;
	} else if (bytes_read == 8 && png_sig_cmp(png_sig, 0, 8) == 0) {
		set_slot(slot, SLOT_IMAGE, read_png_as_framebuffer(fp));
		// since we read the signature, we're expecting a png, if this returns null anyway
		// then the file must be invalid data, so we exit immediately
		if (slots[slot].data == NULL) exit(1);
	} else {
		char *geometry_data = NULL;
		size_t total = 0;
		size_t capacity = 64;
		geometry_data = malloc(capacity);

		memcpy(geometry_data, png_sig, bytes_read);
		total = bytes_read;

		int c;
		while ((c = fgetc(fp)) != EOF) {
			if (total + 2 >= capacity) {
				capacity *= 2;
				geometry_data = realloc(geometry_data, capacity);
			}
			geometry_data[total++] = c;
		}
		geometry_data[total] = '\0';

		struct fbcondecor_geometry* geometry = malloc(sizeof(struct fbcondecor_geometry));
		int consumed = 0;

		if (sscanf(
			geometry_data,
			"%" SCNu16 " %" SCNu16 " %" SCNu16 " %" SCNu16 " %n",
			&geometry->offset_x, &geometry->offset_y, &geometry->width, &geometry->height,
			&consumed
		) != 4) exit(1);
		
		if ((size_t) consumed != total) exit(1);
		
		set_slot(slot, SLOT_GEOMETRY, (u8*) geometry);
	}
}

void write_file(int slot, const char* filename) {
	FILE* fp = NULL;
	if (!strcmp(filename, "-")) fp = fdopen(1, "wb");
	else fp = fopen(filename, "wb");
	if (fp == NULL) exit(1);

	switch (slots[slot].type) {
	case SLOT_NOTHING:
		break;
	case SLOT_GEOMETRY:
		if (!slots[slot].data) break;
		struct fbcondecor_geometry *g = (struct fbcondecor_geometry *) slots[slot].data;
		fprintf(
			fp,
			"%" PRIu16 " %" PRIu16 " %" PRIu16 " %" PRIu16 "\n",
			g->offset_x, g->offset_y, g->width, g->height
		);
		break;
	case SLOT_IMAGE:
		if (!slots[slot].data) break;
		write_framebuffer_as_png(fp, slots[slot].data);
		break;
	}
	fflush(fp);
}


int main(int argc, char **argv)
{

	int fd_tty0 = open(PATH_DEV "/tty0", O_RDWR);

	if (argc == 1) {
		usage();
		exit(1);
	}

	for (int n = 1; n < argc; n++)
		if (!strcmp(argv[n], "--help") || !strcmp(argv[n], "-h")) {
			usage();
			return 0;
		}
	
	fd_fbcondecor = open(FBCON_DECOR_DEV, O_RDWR);
	if (fd_fbcondecor == -1) {
		iprint(MSG_ERROR, "Failed to open the fbcon_decor control device.\n");
		exit(1);
	}
	
	int selected_tty;

	struct vt_stat stat;
	if (ioctl(fd_tty0, VT_GETSTATE, &stat) < 0) exit(1);
	else selected_tty = stat.v_active;
	
	fb_get_settings();

	// dry run, first, then a normal run
	bool execute = false;
	parsing:
	int command = 0;
	for (int i = 1; (int) i < argc; i++) {
		command++;

		if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
			usage();
			return 0;
		}

		else if (!strcmp(argv[i], "set-slot")) {
			require_args(i, argc, 2, command);
			long slot_to_set = parse_slot(argv[++i], command, 1);
			long slot_to_set_to = parse_slot(argv[++i], command, 0);
			slots[slot_to_set].type = slots[slot_to_set_to].type;
			if (!execute) continue;
			slots[slot_to_set].data = slots[slot_to_set_to].data;
		}
		
		else if (!strcmp(argv[i], "read-geometry")) {
			require_args(i, argc, 2, command);
			long slot_to_set = parse_slot(argv[++i], command, 1);
			char *filename = argv[++i];
			slots[slot_to_set].type = SLOT_GEOMETRY;
			if (!execute) continue;
			read_file(slot_to_set, filename);
			if (slots[slot_to_set].type == SLOT_IMAGE) {
				iprint(MSG_ERROR, "RUNTIME ERROR in command %d. Expected a geometry, got an image.\n", command);
				exit(1);
			}
		}
		
		else if (!strcmp(argv[i], "read-image")) {
			require_args(i, argc, 2, command);
			long slot_to_set = parse_slot(argv[++i], command, 1);
			char *filename = argv[++i];
			slots[slot_to_set].type = SLOT_IMAGE;
			if (!execute) continue;
			read_file(slot_to_set, filename);
			if (slots[slot_to_set].type == SLOT_GEOMETRY) {
				iprint(MSG_ERROR, "RUNTIME ERROR in command %d. Expected an image, got a geometry.\n", command);
				exit(1);
			}
		}
		
		else if (!strcmp(argv[i], "write-file")) {
			require_args(i, argc, 2, command);
			long slot_to_write = parse_slot(argv[++i], command, 0);
			char *filename = argv[++i];
			if (!execute) continue;
			write_file(slot_to_write, filename);
		}

		else if (!strcmp(argv[i], "tty")) {
			require_args(i, argc, 1, command);
			long tty = parse_tty(argv[++i], command);
			if (!execute) continue;
			selected_tty = tty;
			if (selected_tty == 0) {
				if (ioctl(fd_tty0, VT_GETSTATE, &stat) < 0) {
					iprint(MSG_ERROR, "RUNTIME ERROR in command %d. Unable to get currently viewed tty.\n", command);
					exit(1);
				}
				else selected_tty = stat.v_active;
			}
		}
		else {
			struct command_ops ops = parse_op(argv[i], command);
			int geometry_slot = -1;
			int image_slot = -1;
			require_args(i, argc, (ops.geometry_op != OP_NONE) + (ops.image_op != OP_NONE), command);
			if (ops.geometry_op != OP_NONE) geometry_slot = parse_slot(argv[++i], command, ops.geometry_op == OP_SET ? 0 : 1);
			if (ops.image_op != OP_NONE) image_slot  = parse_slot(argv[++i], command, ops.image_op == OP_SET ? 0 : 1);

			if (ops.geometry_op == OP_SET || ops.geometry_op == OP_SWAP) {
				if (slots[geometry_slot].type == SLOT_IMAGE) {
					iprint(MSG_ERROR, "Error in geometry slot in command %d. Expected geometry data, got image.\n", command);
					exit(1);
				}
			}

			if (ops.image_op == OP_SET || ops.image_op == OP_SWAP) {
				if (slots[image_slot].type == SLOT_GEOMETRY) {
					iprint(MSG_ERROR, "Error in image slot in command %d. Expected image data, got geometry.\n", command);
					exit(1);
				}
			}
			if (ops.geometry_op == OP_GET || ops.geometry_op == OP_SWAP) slots[geometry_slot].type = SLOT_GEOMETRY;
			if (ops.image_op == OP_GET || ops.image_op == OP_SWAP) slots[image_slot].type = SLOT_IMAGE;

			if (!execute) continue;
			u16 flags = 0;
			flags |= (ops.geometry_op == OP_SET || ops.geometry_op == OP_SWAP) ? FBCONDECOR_FLAG_SET_GEOMETRY : 0;
			flags |= (ops.geometry_op == OP_GET || ops.geometry_op == OP_SWAP) ? FBCONDECOR_FLAG_GET_GEOMETRY : 0;
			flags |= (ops.image_op == OP_SET || ops.image_op == OP_SWAP) ? FBCONDECOR_FLAG_SET_IMAGE : 0;
			flags |= (ops.image_op == OP_GET || ops.image_op == OP_SWAP) ? FBCONDECOR_FLAG_GET_IMAGE : 0;

			struct fbcondecor_geometry* get_geometry = NULL;
			if (flags & FBCONDECOR_FLAG_GET_GEOMETRY)
				get_geometry = malloc(sizeof(struct fbcondecor_geometry));
			
			u8* get_image = NULL;
			if (flags & FBCONDECOR_FLAG_GET_IMAGE)
				get_image = malloc(4 * fbd.var.xres * fbd.var.yres);

			struct fbcondecor_geometry* set_geometry = NULL;
			if (flags & FBCONDECOR_FLAG_SET_GEOMETRY)
				set_geometry = (struct fbcondecor_geometry*) slots[geometry_slot].data;
			
			u8* set_image = NULL;
			if (flags & FBCONDECOR_FLAG_SET_IMAGE)
				set_image = slots[image_slot].data;

			int result = fbcondecor_execute_raw_command(
				selected_tty - 1,
				flags,
				get_geometry,
				set_geometry,
				get_image,
				set_image
			);

			if (result < 0) {
				iprint(MSG_ERROR, "Runtime error in command %d. Fbcondecor ioctl failed with code %d\n", command, result);
				exit(1);
			}
			if (flags & FBCONDECOR_FLAG_GET_GEOMETRY) {
				if (result & FBCONDECOR_STATE_GEOMETRY) slots[geometry_slot].data = (u8*) get_geometry;
				else {
					free(get_geometry);
					slots[geometry_slot].data = NULL;
				}
			}

			if (flags & FBCONDECOR_FLAG_GET_IMAGE) {
				if (result & FBCONDECOR_STATE_IMAGE) slots[image_slot].data = get_image;
				else {
					free(get_image);
					slots[image_slot].data = NULL;
				}
			}
		}
	}
	if (!execute) {
		if (ioctl(fd_tty0, VT_GETSTATE, &stat) < 0) { iprint(MSG_ERROR, "yeah"); exit(1); }
		else selected_tty = stat.v_active;
		execute = true;
		// if i handled everything correctly, then the data would have never been set during parsing in the first place, so this is fine
		for (int i = 0; i < 101; i++) slots[i].type = SLOT_NOTHING;
		goto parsing;
	}

	close(fd_fbcondecor);

	return 0;
}
