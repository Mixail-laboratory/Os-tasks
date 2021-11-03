#include "read_line.h"

struct buffer{
    void* buffer;
    size_t capacity;
    size_t position;
    size_t length;
};


struct file {
    int fd;
    struct buffer *buf;
};

struct buffer* make_buf(size_t capacity) {
    void* buf = malloc(capacity);
    if (buf == NULL)
        return NULL;

    struct buffer* res = malloc(sizeof(struct buffer));
    if (res == NULL) {
        free(buf);
        return NULL;
    }

    res->buffer = buf;
    res->capacity = capacity;
    res->position = 0;
    res->length = 0;
    return res;
}

void free_buffered_file(struct file *f){
    free(f->buf->buffer);
    free(f->buf);
    free(f);
}

struct file *make_buffered_file(int fd, size_t buf_size){
    struct buffer *buf = make_buf(buf_size);
    if (buf == NULL)
        return NULL;

    struct file *f = malloc(sizeof(struct file));
    if (f == NULL) {
        free(buf->buffer);
        free(buf);
        return NULL;
    }

    f->fd = fd;
    f->buf = buf;

    return f;
}

size_t buf_consume_line(struct buffer* buf, char* res, size_t size) {
    size_t cnt = buf->length<= size ? buf->length : size;
    char *buf_start = buf->buffer + buf->position;
    for (size_t i = 0; i < cnt; i++) {
        if (buf_start[i] == '\n') {
            cnt = i + 1;
            break;
        }
    }

    memcpy(res, buf->buffer+ buf->position, cnt);
    buf->position += cnt;
    buf->length -= cnt;
    return cnt;
}

int start_timer(time_t timeout) {
    int timer = timerfd_create(CLOCK_MONOTONIC, 0);
    struct itimerspec ts;

    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    ts.it_value.tv_sec = timeout;
    ts.it_value.tv_nsec = 0;

    timerfd_settime(timer, 0, &ts, NULL);
    return timer;
}

size_t read_line(struct file *f, char *res, size_t size, time_t timeout){
    char *start = res;
    int fd = f->fd;
    struct buffer *buf = f->buf;
    if (size == 0)
        return 0;
    size--; // Reserve space for '\0'.

    int timer = -1;
    if (timeout != -1)
        timer = start_timer(timeout);

    int err = 0;
    while (size) {
        if (!buf->length) {
            size_t bytes_read;
            if (timer == -1) {
                bytes_read = read(fd, buf->buffer, buf->capacity);
            } else {
                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(0, &rfds);
                FD_SET(timer, &rfds);
                int nfds = timer + 1;

                struct timeval tv;
                tv.tv_sec = timeout;
                tv.tv_usec = 0;

                int res = select(nfds, &rfds, NULL, NULL, &tv);
                if (res == -1) {
                    perror("read_line: select failed");
                    err = FAIL;
                    break;
                }
                if (FD_ISSET(0, &rfds))
                    bytes_read = read(fd, buf->buffer, buf->capacity);

                if (FD_ISSET(timer, &rfds)) {
                    err = TIMEOUT;
                    break;;
                }
            }
            if (bytes_read == 0) {
                if (res != start) { // String is not empty
                    *res++ = '\n';
                    break; // Next call to read_line will detect EOF.
                }
                err = MC_EOF;
                break;
            }

            if (bytes_read == -1) {
                perror("read_line: read failed");
                err = FAIL;
                break;
            }
            buf->position = 0;
            buf->length = bytes_read;
        }

        size_t cnt = buf_consume_line(buf, res, size);
        res += cnt;
        size -= cnt;
        if (res[-1] == '\n')
            break;
    }

    if (timer != -1)
        close(timer);

    if (err)
        return err;

    *res = '\0';
    return res - start;
}