#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "fan_output.h"
#if defined(_WIN32) && !defined(O_CLOEXEC)
#define O_CLOEXEC 0
#endif

static int motor_fd = -1;
static int led_fd = -1;

static int write_value(int fd, int value)
{
    ssize_t n;

    do {
        n = write(fd, &value, sizeof(value));
    } while (n < 0 && errno == EINTR);

    if (n != (ssize_t)sizeof(value)) {
        if (n >= 0)
            errno = EIO;

        return -1;
    }

    return 0;
}

static void update_led(int level)
{
    if (led_fd >= 0 && write_value(led_fd, level) < 0) {
        perror("LED 출력 실패 (모터만 사용)");
        close(led_fd);
        led_fd = -1;
    }
}

int fan_output_init(void)
{
    if (motor_fd >= 0) {
        errno = EBUSY;
        return -1;
    }

    motor_fd = open(
        FAN_PWM_DEVICE_PATH,
        O_WRONLY | O_CLOEXEC
    );

    if (motor_fd < 0)
        return -1;

    if (write_value(motor_fd, 0) < 0) {
        int saved = errno;

        close(motor_fd);
        motor_fd = -1;
        errno = saved;

        return -1;
    }

    led_fd = open(FAN_LED_DEVICE_PATH, O_WRONLY | O_CLOEXEC);
    if (led_fd < 0)
        perror("LED 장치 열기 실패 (모터만 사용)");
    else
        update_led(0);
    return 0;
}

int apply_fan_state(const fan_state_t *state)
{
    int expected_speed;

    if (!state || motor_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    switch (state->mode) {
    case FAN_MODE_OFF:
        expected_speed = FAN_SPEED_MIN;
        break;

    case FAN_MODE_S1:
        expected_speed = FAN_SPEED_S1;
        break;

    case FAN_MODE_S2:
        expected_speed = FAN_SPEED_S2;
        break;

    case FAN_MODE_S3:
        expected_speed = FAN_SPEED_S3;
        break;

    default:
        errno = EINVAL;
        return -1;
    }

    if (state->speed != expected_speed ||
        state->power != (expected_speed != 0)) {
        errno = EINVAL;
        return -1;
    }

    /* Motor consumes speed 0/2/5/8; LED consumes mode 0/1/2/3. */
    if (write_value(motor_fd, state->speed) < 0)
        return -1;
    update_led((int)state->mode);
    return 0;
}

void fan_output_cleanup(void)
{
    if (led_fd >= 0) {
        close(led_fd); /* LED release() clears all segments. */
        led_fd = -1;
    }
    if (motor_fd >= 0) {
        /* 드라이버 release()에서도 모터 정지 */
        close(motor_fd);
        motor_fd = -1;
    }
}
