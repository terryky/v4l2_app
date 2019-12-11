#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "util_debug.h"

#define ERRSTR strerror(errno)


struct buffer {
    unsigned int bo_handle;
    void *map_buf;
    int dbuf_fd;
};

static int 
create_drm_buffer (struct buffer *b, int drmfd, uint64_t size)
{
    int  ret;
    void *map_buf = NULL;
    struct drm_mode_create_dumb create_arg = {0};
    struct drm_mode_map_dumb    map_arg    = {0};
    struct drm_prime_handle     prime      = {0};

    create_arg.bpp    = 8;
    create_arg.width  = size;
    create_arg.height = 1;
    ret = drmIoctl (drmfd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg);
    DBG_ASSERT (ret == 0, "DRM_IOCTL_MODE_CREATE_DUMB failed");


    /* MMap DUMB Buffer --> cremap_arg.offset */
    map_arg.handle = create_arg.handle;
    ret = drmIoctl (drmfd, DRM_IOCTL_MODE_MAP_DUMB, &map_arg);
    DBG_ASSERT (ret == 0, "DRM_IOCTL_MODE_MAP_DUMB failed"); 

    /* mmap() --> map */
    map_buf = mmap (0, create_arg.size, PROT_WRITE|PROT_READ , MAP_SHARED, drmfd, map_arg.offset);
    DBG_ASSERT (map_buf != MAP_FAILED, "mmap failed"); 

    prime.handle = create_arg.handle;
    ret = drmIoctl (drmfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime);
    DBG_ASSERT (ret == 0, "PRIME_HANDLE_TO_FD failed: %s\n", ERRSTR);

    b->bo_handle = create_arg.handle;
    b->map_buf = map_buf;
    b->dbuf_fd = prime.fd;

    return 0;
}



int
dump_to_img (char *lpFName, int nW, int nH, void *lpBuf)
{
    FILE *fp;
    char strFName[128];

    sprintf (strFName, "%s_YUNV422_SIZE%dx%d.img", lpFName, nW, nH);

    fp = fopen (strFName, "wb");
    DBG_ASSERT (fp, "fopen failed");

    fwrite (lpBuf, 2, nW * nH, fp);
    fclose (fp);

    fprintf (stderr, "%s\n", strFName);

    return 0;
}

static unsigned int
get_capture_buftype (unsigned int capture_type)
{
    switch (capture_type)
    {
    case V4L2_CAP_VIDEO_CAPTURE_MPLANE: return V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    case V4L2_CAP_VIDEO_CAPTURE:        return V4L2_BUF_TYPE_VIDEO_CAPTURE;
    default:
        DBG_ASSERT (0, "unknown capture_type");
        return 0;
    }
}

static void
print_v4l2_format (struct v4l2_format infmt)
{
    if (infmt.type == V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
        struct v4l2_pix_format fmt = infmt.fmt.pix;
        fprintf (stderr, " WH(%u, %u), 4CC(%.4s), bpl(%d), size(%d)\n",
            fmt.width, fmt.height, (char *)&fmt.pixelformat,
            fmt.bytesperline, fmt.sizeimage);
    }
    else
    {
        fprintf (stderr, "ERR: %s(%d) not support.\n", __FILE__, __LINE__);
    }
}

int
open_drm ()
{
    fprintf (stderr, "-------------------------------------\n");
#if defined (DRM_DRIVER_NAME)
    fprintf (stderr, " open drm: drmOpen(%s)\n", DRM_DRIVER_NAME);
    int fd = drmOpen (DRM_DRIVER_NAME, NULL);
#else
    fprintf (stderr, " open drm: open(/dev/dri/card0)\n");
    int fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
#endif
    fprintf (stderr, "-------------------------------------\n");

    DBG_ASSERT (fd >= 0, "failed to open DRM: %s\n",  ERRSTR);

    return fd;
}

int
open_v4l ()
{
    int fd = open("/dev/video1", O_RDWR | O_CLOEXEC);
    return fd;
}

int main(int argc, char *argv[])
{
    int i, ret;
    unsigned int capture_type = 0;
    unsigned int capture_buftype = 0;
    unsigned int buf_num = 3;
    unsigned int buf_memtype = V4L2_MEMORY_MMAP;   // V4L2_MEMORY_DMABUF;

    int drmfd = open_drm ();
    DBG_ASSERT (drmfd >= 0, "failed to open DRM: %s\n",  ERRSTR);

    int v4lfd = open_v4l ();
    DBG_ASSERT (v4lfd >= 0, "failed to open V4L: %s\n",  ERRSTR);

    struct v4l2_capability caps = {0};
    ret = ioctl (v4lfd, VIDIOC_QUERYCAP, &caps);
    DBG_ASSERT (ret == 0, "VIDIOC_QUERYCAP failed: %s\n", ERRSTR);

    if (caps.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
    {
        fprintf (stderr, "capture type = V4L2_CAP_VIDEO_CAPTURE_MPLANE\n");
        capture_type = V4L2_CAP_VIDEO_CAPTURE_MPLANE;
    }
    else if (caps.capabilities & V4L2_CAP_VIDEO_CAPTURE)
    {
        fprintf (stderr, "capture type = V4L2_BUF_TYPE_VIDEO_CAPTURE\n");
        capture_type = V4L2_CAP_VIDEO_CAPTURE;
    }
    capture_buftype = get_capture_buftype (capture_type);
    DBG_ASSERT (capture_type    != 0, "capture is not supported\n");
    DBG_ASSERT (capture_buftype != 0, "capture is not supported\n");

    struct v4l2_format fmt = {0};
    fmt.type = capture_buftype;
    ret = ioctl (v4lfd, VIDIOC_G_FMT, &fmt);
    DBG_ASSERT (ret == 0, "VIDIOC_G_FMT failed: %s\n", ERRSTR);

    print_v4l2_format (fmt);

    struct v4l2_requestbuffers rqbufs = {0};
    rqbufs.count  = buf_num;
    rqbufs.type   = capture_buftype;
    rqbufs.memory = buf_memtype;

    ret = ioctl (v4lfd, VIDIOC_REQBUFS, &rqbufs);
    DBG_ASSERT (ret == 0, "VIDIOC_REQBUFS failed: %s\n", ERRSTR);
    DBG_ASSERT (rqbufs.count >= buf_num, "VIDIOC_REQBUFS failed");

    struct buffer buffer[buf_num];
    if (buf_memtype == V4L2_MEMORY_DMABUF)
    {
        /* TODO: add support for multiplanar formats */
        uint64_t size  = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;
        for (int i = 0; i < buf_num; i ++)
            ret = create_drm_buffer (&buffer[i], drmfd, size);
    }

    if (buf_memtype == V4L2_MEMORY_MMAP)
    {
        for (i = 0; i < buf_num; i ++)
        {
            struct v4l2_buffer buf = {0};
            buf.type  = capture_buftype;
            buf.index = i; 
            ret = ioctl (v4lfd, VIDIOC_QUERYBUF, &buf);
            DBG_ASSERT (ret == 0, "VIDIOC_QUERYBUF");

            buffer[i].map_buf = mmap (NULL, buf.length, PROT_WRITE|PROT_READ , MAP_SHARED, v4lfd, buf.m.offset);
            DBG_ASSERT (buffer[i].map_buf != MAP_FAILED, "mmap");
        }
    }

    /* initial queue */
    for (i = 1; i < buf_num; i ++) 
    {
        struct v4l2_buffer buf = {0};
        buf.index  = i;
        buf.type   = capture_buftype;
        buf.memory = buf_memtype;

        if (buf_memtype == V4L2_MEMORY_DMABUF)
        {
            if (capture_buftype == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
            {
                struct v4l2_plane plane = {0};
                plane.m.fd = buffer[i].dbuf_fd;
                buf.m.planes = &plane;
                buf.length   = 1;
            }
            else
            {
                buf.m.fd = buffer[i].dbuf_fd;
            }
        }

        ret = ioctl (v4lfd, VIDIOC_QBUF, &buf);
        DBG_ASSERT (ret == 0, "VIDIOC_QBUF for buffer %d failed: %s\n", buf.index, ERRSTR);
    }

    int type = capture_buftype;
    ret = ioctl (v4lfd, VIDIOC_STREAMON, &type);
    DBG_ASSERT (ret == 0, "STREAMON failed: %s\n", ERRSTR);

    struct pollfd fds[] = {
        { .fd = v4lfd, .events = POLLIN },
    };

    while ((ret = poll(fds, 2, 5000)) > 0) 
    {
        if (fds[0].revents & POLLIN) 
        {
            /* dequeue */
            struct v4l2_buffer buf = {0};
            buf.type   = capture_buftype;
            buf.memory = buf_memtype;
            ret = ioctl (v4lfd, VIDIOC_DQBUF, &buf);
            DBG_ASSERT (ret == 0, "VIDIOC_DQBUF failed: %s\n", ERRSTR);

            /* dump to file */
            char strbuf[128];
            static int s_ncnt = 0;
            sprintf (strbuf, "cap_%05d", s_ncnt); 
            dump_to_img (strbuf, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height, buffer[buf.index].map_buf);
            s_ncnt ++;

            /* queue */
            ret = ioctl (v4lfd, VIDIOC_QBUF, &buf);
            DBG_ASSERT (ret == 0, "VIDIOC_QBUF failed: %s\n", ERRSTR);
        }
    }
    return 0;
}

