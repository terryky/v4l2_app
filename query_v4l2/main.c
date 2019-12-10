#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>

#define test_cap(caps, bit)  do {       \
    if (caps & bit)                     \
        fprintf(stderr, "    "#bit"\n");\
} while (0)

static void
dump_capabilities (__u32 caps)
{
    test_cap(caps, V4L2_CAP_VIDEO_CAPTURE);
    test_cap(caps, V4L2_CAP_VIDEO_CAPTURE_MPLANE);
    test_cap(caps, V4L2_CAP_VIDEO_OUTPUT);
    test_cap(caps, V4L2_CAP_VIDEO_OUTPUT_MPLANE);
    test_cap(caps, V4L2_CAP_VIDEO_M2M);
    test_cap(caps, V4L2_CAP_VIDEO_M2M_MPLANE);
    test_cap(caps, V4L2_CAP_VIDEO_OVERLAY);
    test_cap(caps, V4L2_CAP_VBI_CAPTURE);
    test_cap(caps, V4L2_CAP_VBI_OUTPUT);
    test_cap(caps, V4L2_CAP_SLICED_VBI_CAPTURE);
    test_cap(caps, V4L2_CAP_SLICED_VBI_OUTPUT);
    test_cap(caps, V4L2_CAP_RDS_CAPTURE);
    test_cap(caps, V4L2_CAP_VIDEO_OUTPUT_OVERLAY);
    test_cap(caps, V4L2_CAP_HW_FREQ_SEEK);
    test_cap(caps, V4L2_CAP_RDS_OUTPUT);
    test_cap(caps, V4L2_CAP_TUNER);
    test_cap(caps, V4L2_CAP_AUDIO);
    test_cap(caps, V4L2_CAP_RADIO);
    test_cap(caps, V4L2_CAP_MODULATOR);
    test_cap(caps, V4L2_CAP_SDR_CAPTURE);
    test_cap(caps, V4L2_CAP_EXT_PIX_FORMAT);
    test_cap(caps, V4L2_CAP_SDR_OUTPUT);
#if defined (V4L2_CAP_META_CAPTURE)
    test_cap(caps, V4L2_CAP_META_CAPTURE);
#endif
    test_cap(caps, V4L2_CAP_READWRITE);
    test_cap(caps, V4L2_CAP_ASYNCIO);
    test_cap(caps, V4L2_CAP_STREAMING);
#if defined (V4L2_CAP_META_OUTPUT)
    test_cap(caps, V4L2_CAP_META_OUTPUT);
#endif
#if defined (V4L2_CAP_TOUCH)
    test_cap(caps, V4L2_CAP_TOUCH);
#endif
    test_cap(caps, V4L2_CAP_DEVICE_CAPS);
    
}

static int
dump_interval (int v4l_fd, unsigned int format, unsigned int w, unsigned int h)
{
    struct v4l2_frmivalenum frmival = {0};

    frmival.pixel_format = format;
    frmival.width  = w;
    frmival.height = h;
    while (ioctl (v4l_fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) >= 0)
    {
        switch (frmival.type){
        case V4L2_FRMIVAL_TYPE_DISCRETE:
        {
            unsigned int numerator   = frmival.discrete.numerator;
            unsigned int denominator = frmival.discrete.denominator;
            fprintf (stderr, "      Interval(%2d, %2d, %f)\n", numerator, denominator, (float)numerator/(float)denominator);
            break;
        }
        case V4L2_FRMIVAL_TYPE_CONTINUOUS:
        case V4L2_FRMIVAL_TYPE_STEPWISE:
        default:
            fprintf (stderr, "      %s(%d) not support\n", __FILE__, __LINE__);
            break;
        }
        frmival.index ++;
    }

    return 0;
}

static int
dump_resolution (int v4l_fd, unsigned int format)
{
    struct v4l2_frmsizeenum frmsize = {0};

    frmsize.pixel_format = format;
    while (ioctl (v4l_fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0)
    {
        fprintf (stderr, "    Framesize[%d]: ", frmsize.index);
        switch (frmsize.type){
        case V4L2_FRMIVAL_TYPE_DISCRETE:
        {
            unsigned int w = frmsize.discrete.width;
            unsigned int h = frmsize.discrete.height;
            fprintf (stderr, "(discrete  ): (%4d, %4d)\n", w, h);
            dump_interval (v4l_fd, format, w, h);
            break;
        }
        case V4L2_FRMIVAL_TYPE_CONTINUOUS:
        {
            unsigned int min_w = frmsize.stepwise.min_width;
            unsigned int min_h = frmsize.stepwise.min_height;
            unsigned int max_w = frmsize.stepwise.max_width;
            unsigned int max_h = frmsize.stepwise.max_height;
            unsigned int step_w= frmsize.stepwise.step_width;
            unsigned int step_h= frmsize.stepwise.step_height;
            fprintf (stderr, "(continuous): (%4d, %4d)-(%4d, %4d) step(%4d, %4d)\n",
                        min_w, min_h, max_w, max_h, step_w, step_h);
            break;
        }
        case V4L2_FRMIVAL_TYPE_STEPWISE:
        {
            unsigned int w = frmsize.stepwise.max_width;
            unsigned int h = frmsize.stepwise.max_height;
            fprintf (stderr, "(stepwise  ): (%4d, %4d)\n", w, h);
            dump_interval (v4l_fd, format, w, h);
            break;
        }
        default:
            fprintf (stderr, "    %s(%d) not support\n", __FILE__, __LINE__);
        }
        frmsize.index ++;
    }

    return 0;
}

static char *
get_format_name (unsigned int format)
{
    switch (format){
    /* RGB */
    case V4L2_PIX_FMT_RGB332:  return "V4L2_PIX_FMT_RGB332";
    case V4L2_PIX_FMT_RGB444:  return "V4L2_PIX_FMT_RGB444";
    case V4L2_PIX_FMT_RGB555:  return "V4L2_PIX_FMT_RGB555";
    case V4L2_PIX_FMT_RGB565:  return "V4L2_PIX_FMT_RGB565";
    case V4L2_PIX_FMT_RGB555X: return "V4L2_PIX_FMT_RGB555X";
    case V4L2_PIX_FMT_RGB565X: return "V4L2_PIX_FMT_RGB565X";
    case V4L2_PIX_FMT_BGR666:  return "V4L2_PIX_FMT_BGR666";
    case V4L2_PIX_FMT_BGR24:   return "V4L2_PIX_FMT_BGR24";
    case V4L2_PIX_FMT_RGB24:   return "V4L2_PIX_FMT_RGB24";
    case V4L2_PIX_FMT_BGR32:   return "V4L2_PIX_FMT_BGR32";
    case V4L2_PIX_FMT_RGB32:   return "V4L2_PIX_FMT_RGB32";

    /* Grey */
    case V4L2_PIX_FMT_GREY:    return "V4L2_PIX_FMT_GREY";
    case V4L2_PIX_FMT_Y4:      return "V4L2_PIX_FMT_Y4";
    case V4L2_PIX_FMT_Y6:      return "V4L2_PIX_FMT_Y6";
    case V4L2_PIX_FMT_Y10:     return "V4L2_PIX_FMT_Y10";
    case V4L2_PIX_FMT_Y12:     return "V4L2_PIX_FMT_Y12";
    case V4L2_PIX_FMT_Y16:     return "V4L2_PIX_FMT_Y16";

    case V4L2_PIX_FMT_YUYV:    return "V4L2_PIX_FMT_YUYV";
    case V4L2_PIX_FMT_UYVY:    return "V4L2_PIX_FMT_UYVY";
    case V4L2_PIX_FMT_H264:    return "V4L2_PIX_FMT_H264";
    case V4L2_PIX_FMT_JPEG:    return "V4L2_PIX_FMT_JPEG";
    case V4L2_PIX_FMT_MJPEG:   return "V4L2_PIX_FMT_MJPEG";

    default: return "UNKNOWN";
    }
}

static int
dump_video_capture_format (int v4l_fd)
{
    int ret;
    struct v4l2_format  fmt = {0};
    struct v4l2_fmtdesc fmtdesc = {0};


    fprintf (stderr, "----- VIDEO_CAPTURE -----\n");
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(v4l_fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0)
    {
        fprintf (stderr, "%s(%d)\n", __FILE__, __LINE__);
        return ret;
    }

    fprintf (stderr, " WH(%u, %u), 4CC(%.4s)\n",
        fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
        (char *)&fmt.fmt.pix_mp.pixelformat);


    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl (v4l_fd, VIDIOC_ENUM_FMT, &fmtdesc) >= 0)
    {
        fprintf (stderr, " pixelformat[%d]: (%.4s) %s\n",
            fmtdesc.index, (char*)&fmt.fmt.pix_mp.pixelformat,
            get_format_name (fmtdesc.pixelformat));
        dump_resolution (v4l_fd, fmtdesc.pixelformat);

        fmtdesc.index ++;
    }
    return 0;
}

static int
dump_v4l2_device (char *devname)
{
    int fd = open (devname, O_RDWR | O_NONBLOCK | O_CLOEXEC, 0);
    if (fd < 0)
    {
        return -1;
    }
    
    fprintf (stderr, "-----------------------------------\n");
    fprintf (stderr, " %s\n", devname);
    fprintf (stderr, "-----------------------------------\n");

    struct v4l2_capability cap = {0};
    ioctl (fd, VIDIOC_QUERYCAP, &cap);
    fprintf (stderr, " Driver name    : %s\n", cap.driver);
    fprintf (stderr, " Device name    : %s\n", cap.card);
    fprintf (stderr, " Device Location: %s\n", cap.bus_info);

    __u32 ver = cap.version;
    fprintf (stderr, " Driver version : %u.%u.%u\n", 
                (ver >> 16) & 0xFF, (ver >> 8) & 0xFF, ver & 0xFF);

    fprintf (stderr, " Capabilities   : %08x\n", cap.capabilities);
    dump_capabilities (cap.capabilities);
    fprintf (stderr, " Device Caps    : %08x\n", cap.device_caps);
    dump_capabilities (cap.device_caps);

    if (cap.device_caps & V4L2_CAP_VIDEO_CAPTURE)
    {
        dump_video_capture_format (fd);
    }

    close (fd);
    return 0;
}


int
main (int argc, char *argv[])
{
    int i;
    char str_devname[64];
    
    //system ("v4l2-ctl -D");
    
    for (i = 0; ; i ++)
    {
        sprintf (str_devname, "/dev/video%d", i);
        if (dump_v4l2_device (str_devname) < 0)
        {
            if (i == 0)
                fprintf (stderr, "can't open \"%s\"\n", str_devname);
            break;
        }
    }
    
    
    
    return 0;
}
