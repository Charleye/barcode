#ifndef _CAPTURE_H
#define _CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct buffer {
    void *start;
    size_t length;
}buffer_t;

int device_open(void);
void device_init(int fd);
void device_close(int fd);
void capture_start(int fd);
void capture_stop(int fd);

int read_frame(int fd, unsigned char *dst, buffer_t *buffers);
void main_loop(int fd, unsigned char *dst, buffer_t *buffers);

buffer_t* mmap_init(int fd);
void device_uninit(buffer_t *buffers);

void image_process(unsigned char *, int, int);

#ifdef __cplusplus
}
#endif

#endif
