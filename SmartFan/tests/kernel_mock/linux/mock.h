#ifndef FAN_KERNEL_MOCK_H
#define FAN_KERNEL_MOCK_H
/* Minimal host substitutes for unit-testing the LED driver's C logic.
 * This does not validate real Linux headers, GPIO API or module loading. */
#include <stddef.h>
#include <errno.h>
typedef ptrdiff_t ssize_t;
typedef long long loff_t;
struct inode { int unused; };
struct file { int unused; };
struct file_operations {
    void *owner;
    ssize_t (*write)(struct file *, const char *, size_t, loff_t *);
    int (*release)(struct inode *, struct file *);
};
struct miscdevice {
    int minor;
    const char *name;
    const struct file_operations *fops;
};
#define __user
#define __init
#define __exit
#define THIS_MODULE NULL
#define MISC_DYNAMIC_MINOR 255
#define GPIOF_OUT_INIT_LOW 0
#define module_init(fn) extern int fan_mock_module
#define module_exit(fn) extern int fan_mock_module
#define MODULE_LICENSE(value) extern int fan_mock_module
#define MODULE_DESCRIPTION(value) extern int fan_mock_module
int gpio_request_one(int gpio, unsigned long flags, const char *label);
void gpio_set_value(int gpio, int value);
void gpio_free(int gpio);
int misc_register(struct miscdevice *dev);
void misc_deregister(struct miscdevice *dev);
unsigned long copy_from_user(void *to, const void *from, unsigned long size);
#endif
