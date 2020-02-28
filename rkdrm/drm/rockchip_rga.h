/*
 * Copyright (C) 2017 Fuzhou Rcockhip Electronics Co.Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Yakir Yang <ykk@rock-chips.com>
 *    Jacob Chen <jacob2.chen@rock-chips.com>
 */

#ifndef _FIMrga_H_
#define _FIMrga_H_

enum e_rga_op {
	RGA_OP_OVER,
	RGA_OP_CONSTANT,
	RGA_OP_CUSTOM,
};

#define RGA_PLANE_MAX_NR	3
#define RGA_MAX_CMD_NR		32
#define RGA_MAX_GEM_CMD_NR	10
#define RGA_MAX_CMD_LIST_NR     64

struct rga_image {
	unsigned int color_mode;
	unsigned int width;
	unsigned int height;
	unsigned int stride;
	unsigned int fill_color;
	unsigned int buf_type;
	unsigned int bo[RGA_PLANE_MAX_NR];
	struct drm_rockchip_rga_userptr user_ptr[RGA_PLANE_MAX_NR];
};

struct rga_context {
	int fd;
	unsigned int major;
	unsigned int minor;
	struct drm_rockchip_rga_cmd cmd[RGA_MAX_CMD_NR];
	struct drm_rockchip_rga_cmd cmd_buf[RGA_MAX_GEM_CMD_NR];
	unsigned int cmd_nr;
	unsigned int cmd_buf_nr;
	unsigned int cmdlist_nr;
};

struct rga_context *rga_init(int fd);

void rga_fini(struct rga_context *ctx);

int rga_exec(struct rga_context *ctx);

int rga_solid_fill(struct rga_context *ctx, struct rga_image *img,
		   unsigned int x, unsigned int y, unsigned int w,
		   unsigned int h);

int rga_copy(struct rga_context *ctx, struct rga_image *src,
	     struct rga_image *dst, unsigned int src_x,
	     unsigned int src_y, unsigned int dst_x, unsigned int dst_y,
	     unsigned int w, unsigned int h);

int rga_copy_with_scale(struct rga_context *ctx, struct rga_image *src,
			struct rga_image *dst, unsigned int src_x,
			unsigned int src_y, unsigned int src_w,
			unsigned int src_h, unsigned int dst_x,
			unsigned int dst_y, unsigned int dst_w,
			unsigned int dst_h);

int rga_copy_with_rotate(struct rga_context *ctx, struct rga_image *src,
			 struct rga_image *dst, unsigned int src_x,
			 unsigned int src_y, unsigned int src_w,
			 unsigned int src_h, unsigned int dst_x,
			 unsigned int dst_y, unsigned int dst_w,
			 unsigned int dst_h, unsigned int degree);

int rga_blend(struct rga_context *ctx, struct rga_image *src,
	      struct rga_image *dst, unsigned int src_x,
	      unsigned int src_y, unsigned int src_w,
	      unsigned int src_h, unsigned int dst_x,
	      unsigned int dst_y, unsigned int dst_w,
	      unsigned int dst_h, unsigned int degree,
	      enum e_rga_op op, unsigned int arg1, unsigned int arg2);

#endif /* _RGA_H_ */
