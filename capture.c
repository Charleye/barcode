#include <common.h>
#include <sys/mman.h>
#include <capture.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <fcntl.h>
#include <assert.h>

unsigned int buf_count = 2;
unsigned int width = 800;
unsigned int height = 600;
char *device = "/dev/video4";

int main(int argc, char **argv)
{
    int fd;
    buffer_t *buffers = NULL;
    unsigned char *image = NULL;

    if (argc == 2)
        device = argv[1];
    else
        printf("Default device: '%s'\n", device);

    image = calloc(sizeof(unsigned char), width * height * 2);   // YUYV format
    if (!image)
        err_exit(errno, "calloc");

    fd = device_open();
    if (fd < 0)
        err_exit(errno, "device_open");
    device_init(fd);

    buffers = mmap_init(fd);
    capture_start(fd);
    main_loop(fd, image, buffers);
    capture_stop(fd);

    device_uninit(buffers);
    device_close(fd);

    image_process(image, width, height);

    if(image)
        free(image);

    return 0;
}

int device_open(void)
{
    struct stat st;
    int fd;

    if (stat(device, &st) == -1) {
        printf("Cannot identify '%s'\n", device);
        err_exit(errno, "stat");
    }

    if (!S_ISCHR(st.st_mode)) {
        printf("%s is no device\n", device);
        exit(EXIT_FAILURE);
    }

    fd = v4l2_open(device, O_RDWR | O_NONBLOCK, 0);
    if (fd == -1) {
        printf("Cannot open '%s'\n", device);
        err_exit(errno, "v4l2_open");
    }

    return fd;
}

void device_init(int fd)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format format;
    struct v4l2_streamparm frame;

    if (v4l2_ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1) {
        if (errno == EINVAL) {
            printf("%s is no V4L2 device\n", device);
            exit(EXIT_FAILURE);
        } else {
            err_exit(errno, "v4l2_ioctl [VIDIOC_QUERYCAP]");
        }
    }

    memset(&cropcap, 0, sizeof(cropcap));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (!v4l2_ioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;

        if (v4l2_ioctl(fd, VIDIOC_S_CROP, &crop) == -1) {
            switch (errno) {
                case EINVAL:
                    break;
                default:
                    break;
            }
        }
    } else {
        /* error ignored */
    }

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.field = V4L2_FIELD_INTERLACED;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

    if (v4l2_ioctl(fd, VIDIOC_S_FMT, &format) == -1)
        err_exit(errno, "v4l2_ioctl [VIDIOC_S_FMT]");

    if (format.fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
        printf("libv4l didn't accept YUYV format. Can't proceed.\n");

    if (format.fmt.pix.width != width) {
        width = format.fmt.pix.width;
        printf("Image width set to %i by device '%s'.\n", width, device);
    }

    if (format.fmt.pix.height != height) {
        height = format.fmt.pix.height;
        printf("Image height set to %i by device '%s'.\n", height, device);
    }
}

buffer_t* mmap_init(int fd)
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    buffer_t *buffers;
    int n_buffers;

    memset(&req, 0, sizeof(req));
    req.count = buf_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (v4l2_ioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (errno == EINVAL) {
            printf("%s does not support memory mmap\n", device);
            exit(EXIT_FAILURE);
        } else {
            err_exit(errno, "v4l2_ioctl [VIDIOC_REQBUFS]");
        }
    }

    if (req.count != buf_count) {
        buf_count = req.count;
        printf("buf_count set to %d\n", req.count);
    }

    if (req.count < 2) {
        printf("Insufficient buffer memory on %s.\n", device);
        exit(EXIT_FAILURE);
    }

    buffers = calloc(req.count, sizeof(*buffers));
    if(!buffers)
        err_exit(errno, "Out of memory");

    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (v4l2_ioctl(fd, VIDIOC_QUERYBUF, &buf))
            err_exit(errno, "v4l2_ioctl [VIDIOC_QUERYBUF]");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length, PROT_READ |PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if (buffers[n_buffers].start == MAP_FAILED)
            err_exit(errno, "mmap");
    }

    return buffers;
}

void device_uninit(buffer_t *buffers)
{
    int i;

    for(i = 0; i < buf_count; i++) {
        if (v4l2_munmap(buffers[i].start, buffers[i].length) == -1)
            err_exit(errno, "munmap");
    }

    free(buffers);
}

void device_close(int fd)
{
    if (v4l2_close(fd) == -1)
        err_exit(errno, "v4l2_close");
}

void capture_start(int fd)
{
    unsigned int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

    for (i = 0; i < buf_count; i++) {
        memset(&buf, 0, sizeof(buf));

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf))
            err_exit(errno, "v4l2_ioctl [VIDIOC_QBUF]");
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (v4l2_ioctl(fd, VIDIOC_STREAMON, &type) == -1)
        err_exit(errno, "v4l2_ioctl [VIDIOC_STREAMON]");
}

void main_loop(int fd, unsigned char *dst, buffer_t *buffers)
{
    int count = 3;
    unsigned int number_of_timeouts = 0;

    while(count--) {
        for(;;) {
            fd_set fds;
            struct timeval tv;
            int r;

            FD_ZERO(&fds);
            FD_SET(fd, &fds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            r = select(fd + 1, &fds, NULL, NULL, &tv);
            if (r == -1) {
                if (errno == EINTR)
                    continue;
                err_exit(errno, "select");
            }

            if (r == 0) {
                if (number_of_timeouts <= 0) {
                    count++;
                } else {
                    printf("select timeout\n");
                    exit(EXIT_FAILURE);
                }
            }

            if (read_frame(fd, dst, buffers))
                break;
            /* EAGAIN - continue select loop */
        }
    }
}

int read_frame(int fd, unsigned char *dst, buffer_t *buffers)
{
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (v4l2_ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        switch (errno) {
            case EAGAIN:
                return 0;
            case EIO:
            default:
                err_exit(errno, "v4l2_ioctl [VIDIOC_DQBUF]");
        }
    }

    assert(buf.index < buf_count);
    memcpy(dst, (unsigned char *)buffers[buf.index].start, width*height*2);

    if (v4l2_ioctl(fd, VIDIOC_QBUF, &buf) == -1)
        err_exit(errno, "v4l2_ioctl [VIDIOC_QBUF]");

    return 1;
}

void capture_stop(int fd)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (v4l2_ioctl(fd, VIDIOC_STREAMOFF, &type) == -1)
        err_exit(errno, "v4l2_ioctl [VIDIOC_STREAMOFF]");
}
