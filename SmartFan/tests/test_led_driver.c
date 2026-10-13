#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../kernel/fan_led/fan_led_drv.c"

static int requests, frees, registered, deregistered;
static int fail_gpio = -1, fail_register, fail_copy;
static int outputs[8];
static int index_of(int gpio)
{
    for (int i = 0; i < 8; ++i) if (led_gpio[i] == gpio) return i;
    assert(0);
    return -1;
}
int gpio_request_one(int gpio, unsigned long flags, const char *label)
{
    assert(index_of(gpio) == requests);
    assert(flags == GPIOF_OUT_INIT_LOW && strcmp(label, "fan_led") == 0);
    if (requests++ == fail_gpio) return -EBUSY;
    outputs[index_of(gpio)] = 0;
    return 0;
}
void gpio_set_value(int gpio, int value) { outputs[index_of(gpio)] = value; }
void gpio_free(int gpio) { assert(index_of(gpio) >= 0); ++frees; }
int misc_register(struct miscdevice *dev)
{
    assert(strcmp(dev->name, "fan_led") == 0);
    if (fail_register) return -EBUSY;
    ++registered;
    return 0;
}
void misc_deregister(struct miscdevice *dev) { assert(dev == &led_dev); ++deregistered; }
unsigned long copy_from_user(void *to, const void *from, unsigned long size)
{
    if (fail_copy) return size;
    memcpy(to, from, size);
    return 0;
}
static void expect_lights(int count)
{
    for (int i = 0; i < 8; ++i) assert(outputs[i] == (i < count));
}
int main(void)
{
    const int expected[] = {492, 398, 483, 474, 482, 481, 472, 454};
    for (int i = 0; i < 8; ++i) assert(led_gpio[i] == expected[i]);
    assert(led_init() == 0 && requests == 8 && registered == 1);
    expect_lights(0);
    for (int level = 0; level < 4; ++level) {
        assert(led_write(NULL, (const char *)&level, sizeof(level), NULL) ==
               (ssize_t)sizeof(level));
        expect_lights(led_cnt[level]);
    }
    for (int level = -1; level <= 4; level += 5) {
        assert(led_write(NULL, (const char *)&level, sizeof(level), NULL) == -EINVAL);
        expect_lights(8);
    }
    int level = 0;
    assert(led_write(NULL, (const char *)&level, 1, NULL) == -EINVAL);
    fail_copy = 1;
    assert(led_write(NULL, (const char *)&level, sizeof(level), NULL) == -EFAULT);
    expect_lights(8);
    fail_copy = 0;
    assert(led_release(NULL, NULL) == 0);
    expect_lights(0);
    led_show(3);
    led_exit();
    assert(frees == 8 && deregistered == 1);
    expect_lights(0);
    for (fail_gpio = 0; fail_gpio < 8; ++fail_gpio) {
        requests = frees = 0;
        assert(led_init() == -EBUSY && frees == fail_gpio);
    }
    requests = frees = 0; fail_gpio = -1; fail_register = 1;
    assert(led_init() == -EBUSY && frees == 8);
    puts("PASS: LED driver logic with mocked kernel API: pins, 0/2/5/8 lights, invalid writes, OFF and resource failures");
    return 0;
}
