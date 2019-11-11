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
    test_cap(caps, V4L2_CAP_TOUCH);
    test_cap(caps, V4L2_CAP_DEVICE_CAPS);
    
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
