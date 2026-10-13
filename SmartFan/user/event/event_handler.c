#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "event_handler.h"
#if defined(_WIN32) && !defined(O_CLOEXEC)
#define O_CLOEXEC 0 
#endif

struct encoder_sample { int steps; unsigned int pressed; };
_Static_assert(sizeof(struct encoder_sample) == 8, "encoder ABI size");
static int sensor_fd = -1, encoder_fd = -1;
static bool automatic_mode, near_sensor, auto_blocked;
static bool raw_pressed, stable_pressed, long_sent;
static struct timespec raw_changed, press_started;
static fan_event_t pending[8];
static unsigned int pending_count, pending_index;

static long elapsed_ms(struct timespec now, struct timespec start)
{
    long seconds = (long)(now.tv_sec - start.tv_sec);
    long nanoseconds = now.tv_nsec - start.tv_nsec;
    if (nanoseconds < 0) {
        --seconds;
        nanoseconds += 1000000000L;
    }
    return seconds * 1000L + nanoseconds / 1000000L;
}

void event_handler_cleanup(void)
{
    if (sensor_fd >= 0) close(sensor_fd);
    if (encoder_fd >= 0) close(encoder_fd);
    sensor_fd = encoder_fd = -1;
    near_sensor = auto_blocked = false;
    raw_pressed = stable_pressed = long_sent = false;
    raw_changed = press_started = (struct timespec){0};
    pending_count = pending_index = 0;
}

int event_handler_configure(bool automatic)
{
    if (encoder_fd >= 0) { errno = EBUSY; return -1; }
    automatic_mode = automatic;
    return 0;
}

static int initialize(void)
{
    int saved;
    encoder_fd = open(ENCODER_DEVICE_PATH, O_RDONLY | O_CLOEXEC);
    if (encoder_fd < 0) return -1;
    if (automatic_mode) {
        sensor_fd = open(ULTRASONIC_DEVICE_PATH, O_RDONLY | O_CLOEXEC);
        if (sensor_fd < 0) {
            saved = errno;
            event_handler_cleanup();
            errno = saved;
            return -1;
        }
    }
    return 0;
}

static void enqueue(fan_event_t event)
{
    pending[pending_count++] = event;
}

static fan_event_t button_event(struct encoder_sample sample,
                                struct timespec now)
{
    bool pressed = sample.pressed != 0;
    if (pressed != raw_pressed) {
        raw_pressed = pressed;
        raw_changed = now;
    }
    if (pressed != stable_pressed && elapsed_ms(now, raw_changed) >= 30) {
        stable_pressed = pressed;
        if (pressed) {
            press_started = raw_changed;
            long_sent = false;
        } else {
            if (!long_sent && elapsed_ms(raw_changed, press_started) >=
                              FAN_LONG_PRESS_MS) {
                long_sent = true;
                return EVENT_LONG_PRESS;
            }
            if (!long_sent) return EVENT_POWER_TOGGLE;
        }
    }
    /* A release being debounced must not turn a <2s press into a long one. */
    if (stable_pressed && raw_pressed && !long_sent &&
        elapsed_ms(now, press_started) >= FAN_LONG_PRESS_MS) {
        long_sent = true;
        return EVENT_LONG_PRESS;
    }
    return EVENT_NONE;
}

fan_event_t get_event(void)
{
    struct encoder_sample sample;
    struct timespec now;
    fan_event_t button;
    ssize_t n;
    int distance_mm;

    if (pending_index < pending_count) return pending[pending_index++];
    pending_count = pending_index = 0;
    if (encoder_fd < 0 && initialize() < 0) return EVENT_ERROR;
    n = read(encoder_fd, &sample, sizeof(sample));
    if (n != (ssize_t)sizeof(sample)) {
        if (n >= 0) errno = EIO;
        return EVENT_ERROR;
    }
    if (sample.pressed > 1) { errno = EIO; return EVENT_ERROR; }
    if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) return EVENT_ERROR;
    button = button_event(sample, now);

    if (automatic_mode) {
        n = read(sensor_fd, &distance_mm, sizeof(distance_mm));
        if (n != (ssize_t)sizeof(distance_mm)) {
            if (n < 0 && (errno == ETIMEDOUT || errno == EBUSY)) {
                near_sensor = false;
                return EVENT_SENSOR_LOST;
            }
            if (n >= 0) errno = EIO;
            return EVENT_ERROR;
        }
        if (distance_mm <= 0 || distance_mm > 4000) {
            near_sensor = false;
            return EVENT_SENSOR_LOST;
        }
        if (distance_mm > 200) {
            bool was_near = near_sensor;
            near_sensor = auto_blocked = false;
            return was_near ? EVENT_FAR : EVENT_NONE;
        }
        if (!near_sensor && !auto_blocked) enqueue(EVENT_NEAR);
        near_sensor = true;
    }

    if (button != EVENT_NONE) {
        enqueue(button);
        if (automatic_mode) {
            if (button == EVENT_LONG_PRESS) auto_blocked = true;
            else auto_blocked = !auto_blocked;
        }
    }
    /* Press/release debounce also suppress contact-induced rotation. */
    if (!raw_pressed && !stable_pressed && button == EVENT_NONE &&
        (!automatic_mode || !auto_blocked)) {
        /* More than two steps has the same effect on our saturated states. */
        int count = sample.steps > 0 ? (sample.steps > 1 ? 2 : 1) :
                    (sample.steps < 0 ? (sample.steps < -1 ? 2 : 1) : 0);
        while (count-- > 0)
            enqueue(sample.steps > 0 ? EVENT_SPEED_UP : EVENT_SPEED_DOWN);
    }
    return pending_count ? pending[pending_index++] : EVENT_NONE;
}
