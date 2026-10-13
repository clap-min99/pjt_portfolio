#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int fake_open(const char *path, int flags, ...);
static int fake_close(int fd);
static ssize_t fake_read(int fd, void *buf, size_t count);
static int fake_clock(clockid_t id, struct timespec *now);
#define open fake_open
#define close fake_close
#define read fake_read
#define clock_gettime fake_clock
/* Compile the production decoder against deterministic device I/O. */
#include "../user/event/event_handler.c"
#undef open
#undef close
#undef read
#undef clock_gettime

static int ms, input_steps, pressed, distance = 100;
static int sensor_error, short_read, open_error, reads, sensor_opens;
static int fake_open(const char *path, int flags, ...)
{
    (void)flags;
    if (open_error) { errno = ENOENT; return -1; }
    if (strcmp(path, ENCODER_DEVICE_PATH) == 0) return 10;
    assert(strcmp(path, ULTRASONIC_DEVICE_PATH) == 0);
    ++sensor_opens;
    return 11;
}
static int fake_close(int fd) { assert(fd == 10 || fd == 11); return 0; }
static ssize_t fake_read(int fd, void *buf, size_t count)
{
    ++reads;
    if (short_read) return 0;
    if (fd == 10) {
        struct encoder_sample s = {input_steps, (unsigned int)pressed};
        assert(count == sizeof(s));
        memcpy(buf, &s, count);
    } else {
        assert(fd == 11 && count == sizeof(distance));
        if (sensor_error) { errno = sensor_error; return -1; }
        memcpy(buf, &distance, count);
    }
    return (ssize_t)count;
}
static int fake_clock(clockid_t id, struct timespec *now)
{
    assert(id == CLOCK_MONOTONIC);
    now->tv_sec = ms / 1000;
    now->tv_nsec = (ms % 1000) * 1000000L;
    return 0;
}
static fan_event_t sample_at(int time_ms, int key, int steps_value)
{
    ms = time_ms; pressed = key; input_steps = steps_value;
    return get_event();
}
static void reset(bool automatic)
{
    event_handler_cleanup();
    sensor_error = short_read = open_error = sensor_opens = reads = 0;
    distance = 100;
    assert(event_handler_configure(automatic) == 0);
}
int main(void)
{
    assert(elapsed_ms((struct timespec){3, 0},
                      (struct timespec){1, 500000}) == 1999);
    reset(false);
    assert(sample_at(0, 0, 2) == EVENT_SPEED_UP);
    assert(get_event() == EVENT_SPEED_UP && reads == 1);
    assert(sample_at(10, 0, -10000) == EVENT_SPEED_DOWN);
    assert(get_event() == EVENT_SPEED_DOWN && reads == 2);
    assert(sensor_opens == 0);
    assert(event_handler_configure(true) == -1 && errno == EBUSY);

    assert(sample_at(100, 1, 0) == EVENT_NONE);
    assert(sample_at(130, 1, 0) == EVENT_NONE);
    assert(sample_at(200, 0, 0) == EVENT_NONE);
    assert(sample_at(230, 0, 0) == EVENT_POWER_TOGGLE);
    assert(sample_at(300, 0, 0) == EVENT_NONE);
    /* Contact bounce below 30ms is ignored. */
    assert(sample_at(310, 1, 0) == EVENT_NONE);
    assert(sample_at(320, 0, 0) == EVENT_NONE);
    assert(sample_at(360, 0, 0) == EVENT_NONE);
    assert(sample_at(400, 1, 0) == EVENT_NONE);
    assert(sample_at(430, 1, 0) == EVENT_NONE);
    assert(sample_at(2400, 1, 0) == EVENT_LONG_PRESS);
    assert(sample_at(2500, 1, 0) == EVENT_NONE);
    assert(sample_at(2600, 0, 0) == EVENT_NONE);
    assert(sample_at(2630, 0, 0) == EVENT_NONE);
    assert(sample_at(3000, 1, 0) == EVENT_NONE);
    assert(sample_at(3030, 1, 0) == EVENT_NONE);
    assert(sample_at(4999, 0, 0) == EVENT_NONE);
    assert(sample_at(5030, 0, 0) == EVENT_POWER_TOGGLE);
    assert(sample_at(6000, 1, 0) == EVENT_NONE);
    assert(sample_at(6030, 1, 0) == EVENT_NONE);
    assert(sample_at(8000, 0, 0) == EVENT_NONE);
    assert(sample_at(8030, 0, 0) == EVENT_LONG_PRESS);

    reset(true);
    assert(sample_at(0, 0, 2) == EVENT_NEAR);
    assert(get_event() == EVENT_SPEED_UP);
    assert(get_event() == EVENT_SPEED_UP && reads == 2);
    assert(sample_at(10, 0, 0) == EVENT_NONE);
    distance = 201;
    assert(sample_at(20, 0, 0) == EVENT_FAR);
    assert(sample_at(30, 0, 0) == EVENT_NONE);
    distance = 200;
    assert(sample_at(40, 0, 0) == EVENT_NEAR);
    assert(sample_at(100, 1, 0) == EVENT_NONE);
    assert(sample_at(140, 1, 0) == EVENT_NONE);
    assert(sample_at(2100, 1, 0) == EVENT_LONG_PRESS);
    sensor_error = ETIMEDOUT;
    assert(sample_at(2200, 1, 0) == EVENT_SENSOR_LOST);
    sensor_error = 0;
    assert(sample_at(2300, 0, 0) == EVENT_NONE);
    assert(sample_at(2340, 0, 0) == EVENT_NONE);
    distance = 201;
    assert(sample_at(2400, 0, 0) == EVENT_FAR);
    distance = 100;
    assert(sample_at(2500, 0, 0) == EVENT_NEAR);
    distance = 0;
    assert(sample_at(2600, 0, 0) == EVENT_SENSOR_LOST);
    short_read = 1;
    assert(sample_at(2700, 0, 0) == EVENT_ERROR && errno == EIO);
    reset(true);
    assert(sample_at(0, 0, 0) == EVENT_NEAR);
    assert(sample_at(100, 1, 0) == EVENT_NONE);
    assert(sample_at(130, 1, 0) == EVENT_NONE);
    assert(sample_at(200, 0, 0) == EVENT_NONE);
    assert(sample_at(230, 0, 0) == EVENT_POWER_TOGGLE);
    assert(sample_at(300, 0, 2) == EVENT_NONE);
    assert(sample_at(400, 1, 0) == EVENT_NONE);
    assert(sample_at(430, 1, 0) == EVENT_NONE);
    assert(sample_at(500, 0, 0) == EVENT_NONE);
    assert(sample_at(530, 0, 0) == EVENT_POWER_TOGGLE);
    assert(sample_at(600, 0, 1) == EVENT_SPEED_UP);
    reset(false);
    open_error = 1;
    assert(sample_at(0, 0, 0) == EVENT_ERROR && errno == ENOENT);
    event_handler_cleanup();
    puts("PASS: encoder queue, debounce, 2s boundary, auto distance and I/O errors");
    return 0;
}
