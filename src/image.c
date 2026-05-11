/*
 * Copyright (C) 2004-2005, Michal Januszewski <spock@gentoo.org>  (original image.c)
 * Copyright (C) 2004-2008, Michal Januszewski <spock@gentoo.org>  (original render.c)
 * Copyright (C) 2026-2026, NyteLyte <nytelyte@tuta.io>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License v2.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */

#include <linux/types.h>   // for __u32
#include <device.h>        // for fbd
#include <image.h>         // for read_png_as_framebuffer, write_framebuffer...
#include <linux/fb.h>      // for fb_var_screeninfo, fb_bitfield
#include <png.h>           // for PNG_LIBPNG_VER_STRING, png_create_info_struct
#include <setjmp.h>        // for setjmp
#include <stdio.h>         // for NULL, FILE
#include <stdbool.h>       // for bool
#include <stdlib.h>        // for free, malloc
#include <types.h>         // for fb_data, u8, u32

/*
 * Lower level graphics fucntions
 */
static inline void put_pixel(u8 r, u8 g, u8 b, u8 *dst)
{
	if (fbd.opt) {
			dst[fbd.ro] = r;
			dst[fbd.go] = g;
			dst[fbd.bo] = b;
	} else {
   	u32 final_r, final_g, final_b;

    if (fbd.var.red.length >= 8)   final_r = (u32)r << (fbd.var.red.length - 8);
    else                           final_r = (u32)r >> (8 - fbd.var.red.length);

    if (fbd.var.green.length >= 8) final_g = (u32)g << (fbd.var.green.length - 8);
    else                           final_g = (u32)g >> (8 - fbd.var.green.length);

    if (fbd.var.blue.length >= 8)  final_b = (u32)b << (fbd.var.blue.length - 8);
    else                           final_b = (u32)b >> (8 - fbd.var.blue.length);

    *(u32*)dst = (
    		(final_r << fbd.var.red.offset) |
    		(final_g << fbd.var.green.offset) |
    		(final_b << fbd.var.blue.offset)
    );
	}
}

static inline void get_pixel(u8 *r, u8 *g, u8 *b, const u8 *src)
{
    if (fbd.opt) {
        *r = src[fbd.ro];
        *g = src[fbd.go];
        *b = src[fbd.bo];
    } else {
        u32 raw = *(const u32 *)src;

        u32 bit_r = (raw >> fbd.var.red.offset)   & ((1 << fbd.var.red.length)   - 1);
        u32 bit_g = (raw >> fbd.var.green.offset) & ((1 << fbd.var.green.length) - 1);
        u32 bit_b = (raw >> fbd.var.blue.offset)  & ((1 << fbd.var.blue.length)  - 1);

        if (fbd.var.red.length >= 8)   *r = (u8)(bit_r >> (fbd.var.red.length - 8));
        else                           *r = (u8)(bit_r << (8 - fbd.var.red.length));

        if (fbd.var.green.length >= 8) *g = (u8)(bit_g >> (fbd.var.green.length - 8));
        else                           *g = (u8)(bit_g << (8 - fbd.var.green.length));

        if (fbd.var.blue.length >= 8)  *b = (u8)(bit_b >> (fbd.var.blue.length - 8));
        else                           *b = (u8)(bit_b << (8 - fbd.var.blue.length));
    }
}

u8* read_png_as_framebuffer(FILE* fp) {

    if (!fp)
    	return NULL;
    
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    u8* volatile data = NULL;
    u8* volatile row_buf = NULL;

    if (!png_ptr || !info_ptr) return NULL;
    
    if (setjmp(png_jmpbuf(png_ptr))) {
    		if (data) free(data);
        if (row_buf) free(row_buf);
        if (png_ptr) png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return NULL;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    png_set_expand(png_ptr);
    png_set_strip_16(png_ptr);
    png_set_gray_to_rgb(png_ptr);
    png_color_16 my_bg = {0, 0, 0, 0, 0}; 
    png_set_background(png_ptr, &my_bg, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1.0);
    png_set_add_alpha(png_ptr, 0xff, PNG_FILLER_AFTER);
    png_read_update_info(png_ptr, info_ptr);

    unsigned int width = png_get_image_width(png_ptr, info_ptr);
    unsigned int height = png_get_image_height(png_ptr, info_ptr);

    data = malloc(width * height * 4);
    row_buf = malloc(png_get_rowbytes(png_ptr, info_ptr));

    for (unsigned int i = 0; i < height; i++) {
        png_read_row(png_ptr, row_buf, NULL);
        u8 *dst_row = data + width * 4 * i;
				for (unsigned int j = 0; j < width; j++)
					put_pixel(row_buf[j*4], row_buf[j*4+1], row_buf[j*4+2], dst_row + (j*4));
    }

    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    free(row_buf);

    return data;
}

int write_framebuffer_as_png(FILE* fp, u8 *data, bool swap_dimensions) {
    if (!fp) return -1;
    if (!data) return -1;

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    u8* volatile row = NULL;

    if (setjmp(png_jmpbuf(png_ptr))) {
				if (row) free(row);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return -1;
    }

    png_init_io(png_ptr, fp);

    unsigned int xres = swap_dimensions ? fbd.var.yres : fbd.var.xres;
    unsigned int yres = swap_dimensions ? fbd.var.xres : fbd.var.yres;

    png_set_IHDR(png_ptr, info_ptr, xres, yres, 8, 
                 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, 
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(png_ptr, info_ptr);

    row = malloc(xres * 4);

    for (u32 y = 0; y < yres; y++) {
        u8 *src_row = data + (xres * 4 * y);
        
        for (__u32 x = 0; x < xres; x++) {
            u8 r, g, b;
            get_pixel(&r, &g, &b, src_row + (x * 4));
            
            row[x*4 + 0] = r;
            row[x*4 + 1] = g;
            row[x*4 + 2] = b;
            row[x*4 + 3] = 0xff;
        }
        png_write_row(png_ptr, row);
    }

    png_write_end(png_ptr, NULL);
    free(row);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 0;
}
