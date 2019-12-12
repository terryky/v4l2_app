#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "util_debug.h"
#include "util_v4l2.h"


int
dump_to_img (char *lpFName, int nW, int nH, unsigned int fmt, void *lpBuf)
{
    FILE *fp;
    char strFName[128];
    int bpp = 1;

    switch (fmt)
    {
    default:
    case v4l2_fourcc('G', 'R', 'E', 'Y'):
        sprintf (strFName, "%s_I8_SIZE%dx%d.img", lpFName, nW, nH);
        bpp = 1;
        break;
    case v4l2_fourcc('Y', 'U', 'Y', 'V'):
        sprintf (strFName, "%s_YUNV422_SIZE%dx%d.img", lpFName, nW, nH);
        bpp = 2;
        break;
    }

    fp = fopen (strFName, "wb");
    DBG_ASSERT (fp, "fopen failed");

    fwrite (lpBuf, bpp, nW * nH, fp);
    fclose (fp);

    fprintf (stderr, "%s\n", strFName);

    return 0;
}


int main(int argc, char *argv[])
{
    capture_dev_t *cap_dev;
    int cap_devid = -1;
    int cap_w, cap_h;
    unsigned int cap_fmt;

    const struct option long_options[] = {
        {"devid",  required_argument, NULL, 'd'},
        {0, 0, 0, 0},
    };

    int c, option_index;
    while ((c = getopt_long (argc, argv, "d:",
                             long_options, &option_index)) != -1)
    {
        switch (c)
        {
        case 'd': cap_devid  = atoi (optarg); break;
        case '?':
            return -1;
        }
    }

    cap_dev = v4l2_open_capture_device (cap_devid);
    DBG_ASSERT (cap_dev, "failed to open V4L\n");

    v4l2_get_capture_wh (cap_dev, &cap_w, &cap_h);
    v4l2_get_capture_pixelformat (cap_dev, &cap_fmt);

    v4l2_show_current_capture_settings (cap_dev);

    v4l2_start_capture (cap_dev);

    while (1)
    {
        capture_frame_t *frame = v4l2_acquire_capture_frame (cap_dev);

        /* dump to file */
        char strbuf[128];
        static int s_ncnt = 0;
        sprintf (strbuf, "cap_%05d", s_ncnt);
        dump_to_img (strbuf, cap_w, cap_h, cap_fmt, frame->vaddr);
        s_ncnt ++;

        v4l2_release_capture_frame (cap_dev, frame);
    }

    return 0;
}

