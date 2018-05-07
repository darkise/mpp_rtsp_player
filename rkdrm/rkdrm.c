#include "rkdrm.h"
#include "bo.h"
#include "dev.h"
#include "modeset.h"

#include "mpp_buffer.h"

#define CODEC_ALIGN(x, a)   (((x)+(a)-1)&~((a)-1))
#define rkdrm_log(fmt,...)\
            do {\
                printf(fmt,##__VA_ARGS__);\
            } while(0)

struct sp_dev *mDev = NULL;
struct sp_plane **mPlanes;
struct sp_crtc *mCrtc;
struct sp_plane *mTestPlane;

int rkdrm_init()
{
    int i, ret = 0;

    mDev = create_sp_dev();
    if (!mDev) {
        rkdrm_log("failed to exec create_sp_dev.\n");
        return -1;
    }

    ret = initialize_screens(mDev);
    if (ret != 0) {
        rkdrm_log("failed to exec initialize_screens.\n");
        return -2;
    }

    mPlanes = (struct sp_plane **)calloc(mDev->num_planes, sizeof(*mPlanes));
    if (!mPlanes) {
        rkdrm_log("failed to calloc mPlanes.\n");
        return -3;
    }

    mCrtc = &mDev->crtcs[0];
    for (i = 0; i < mCrtc->num_planes; i++) {
        mPlanes[i] = get_sp_plane(mDev, mCrtc);
        if (is_supported_format(mPlanes[i], DRM_FORMAT_NV12))
            mTestPlane = mPlanes[i];
    }

    if (!mTestPlane) {
        rkdrm_log("failed to get mTestPlane.\n");
        return -4;
    }

    return 0;
}

int rkdrm_display(MppFrame frame)
{
    struct sp_bo *bo;
    uint32_t handles[4], pitches[4], offsets[4];
    int width, height;
    int /*frm_size, */ret, fd, err;

    err = mpp_frame_get_errinfo(frame) |
          mpp_frame_get_discard(frame);
    if (err) {
        rkdrm_log("get err info %d discard %d, go back.\n",
                mpp_frame_get_errinfo(frame),
                mpp_frame_get_discard(frame));
        return -1;
    }

    width = mpp_frame_get_width(frame);
    height = mpp_frame_get_height(frame);
    width = CODEC_ALIGN(width, 16);
    height = CODEC_ALIGN(height, 16);
    //frm_size = width * height * 3 / 2;
    fd = mpp_buffer_get_fd(mpp_frame_get_buffer(frame));

    bo = (struct sp_bo *)calloc(1, sizeof(struct sp_bo));
    if (!bo) {
        rkdrm_log("failed to calloc bo.\n");
        return -2;
    }

    drmPrimeFDToHandle(mDev->fd, fd, &bo->handle);
    bo->dev = mDev;
    bo->width = width;
    bo->height = height;
    bo->depth = 16;
    bo->bpp = 32;
    bo->format = DRM_FORMAT_NV12;
    bo->flags = 0;

    handles[0] = bo->handle;
    pitches[0] = width;
    offsets[0] = 0;
    handles[1] = bo->handle;
    pitches[1] = width;
    offsets[1] = width * height;
    ret = drmModeAddFB2(mDev->fd, bo->width, bo->height,
                        bo->format, handles, pitches, offsets,
                        &bo->fb_id, bo->flags);
    if (ret != 0) {
        rkdrm_log("failed to exec drmModeAddFb2.\n");
        return -3;
    }

    ret = drmModeSetPlane(mDev->fd, mTestPlane->plane->plane_id,
                          mCrtc->crtc->crtc_id, bo->fb_id, 0,
                          0, 0,   // display x, y
                          960,//mCrtc->crtc->mode.hdisplay, // display width
                          540,//mCrtc->crtc->mode.vdisplay, // display height
                          0, 0, bo->width << 16, bo->height << 16);
    if (ret) {
        rkdrm_log("failed to exec drmModeSetPlane.\n");
        return -3;
    }

    if (mTestPlane->bo) {
        if (mTestPlane->bo->fb_id) {
            ret = drmModeRmFB(mDev->fd, mTestPlane->bo->fb_id);
            if (ret)
                rkdrm_log("failed to exec drmModeRmFB.\n");
        }
        if (mTestPlane->bo->handle) {
            struct drm_gem_close req = {
                    .handle = mTestPlane->bo->handle,
            };

            drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &req);
        }
        free(mTestPlane->bo);
    }
    mTestPlane->bo = bo;

    return 0;
}

void rkdrm_fini()
{
    if (mDev) destroy_sp_dev(mDev);
    if (mPlanes) free(mPlanes);
}
