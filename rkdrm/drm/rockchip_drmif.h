/*
 * Copyright (C) ROCKCHIP, Inc.
 * Author:yzq<yzq@rock-chips.com>
 *
 * based on exynos_drmif.h
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
 */

#ifndef ROCKCHIP_DRMIF_H_
#define ROCKCHIP_DRMIF_H_

#include <xf86drm.h>
#include <stdint.h>
#include "rockchip_drm.h"

struct rockchip_device {
	int fd;
};

/*
 * Rockchip Buffer Object structure.
 *
 * @dev: rockchip device object allocated.
 * @handle: a gem handle to gem object created.
 * @flags: indicate memory allocation and cache attribute types.
 * @size: size to the buffer created.
 * @vaddr: user space address to a gem buffer mmaped.
 * @name: a gem global handle from flink request.
 */
struct rockchip_bo {
	struct rockchip_device *dev;
	uint32_t handle;
	uint32_t flags;
	size_t size;
	void *vaddr;
	uint32_t name;
};

/*
 * device related functions:
 */
struct rockchip_device *rockchip_device_create(int fd);
void rockchip_device_destroy(struct rockchip_device *dev);

/*
 * buffer-object related functions:
 */
struct rockchip_bo *rockchip_bo_create(struct rockchip_device *dev,
				       size_t size, uint32_t flags);
int rockchip_bo_get_info(struct rockchip_device *dev, uint32_t handle,
			 size_t * size, uint32_t * flags);
void rockchip_bo_destroy(struct rockchip_bo *bo);
struct rockchip_bo *rockchip_bo_from_name(struct rockchip_device *dev,
					  uint32_t name);
int rockchip_bo_get_name(struct rockchip_bo *bo, uint32_t * name);
uint32_t rockchip_bo_handle(struct rockchip_bo *bo);
struct rockchip_bo *rockchip_bo_from_handle(struct rockchip_device *dev,
					    uint32_t handle, uint32_t flags,
					    uint32_t size);
void *rockchip_bo_map(struct rockchip_bo *bo);
#endif /* ROCKCHIP_DRMIF_H_ */
