#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/atomic.h>

#include "fan_common.h"

/* 초음파: 물리 11번 Trig, 물리 36번 Echo */
#define GPIO_TRIG 460
#define GPIO_ECHO 461

#define ECHO_TIMEOUT_MS 50
#define MEASURE_INTERVAL_MS 60

struct ultrasonic_device {
    int irq;
    struct miscdevice misc;

    struct mutex measure_lock;
    spinlock_t irq_lock;
    struct completion done;

    bool measuring;
    bool rising_seen;
    s64 start_ns;
    s64 pulse_ns;
};

static struct ultrasonic_device sensor;

/* ---------- 초음파 기능 ---------- */

static irqreturn_t ultrasonic_irq(int irq, void *data)
{
    struct ultrasonic_device *dev = data;
    unsigned long flags;
    s64 now = ktime_get_ns();
    int level = gpio_get_value(GPIO_ECHO);

    spin_lock_irqsave(&dev->irq_lock, flags);

    if (dev->measuring) {
        if (level && !dev->rising_seen) {
            dev->start_ns = now;
            dev->rising_seen = true;
        } else if (!level && dev->rising_seen) {
            dev->pulse_ns = now - dev->start_ns;
            dev->measuring = false;
            complete(&dev->done);
        }
    }

    spin_unlock_irqrestore(&dev->irq_lock, flags);

    return IRQ_HANDLED;
}

static int ultrasonic_open(struct inode *inode, struct file *file)
{
    file->private_data = &sensor;
    return nonseekable_open(inode, file);
}

static ssize_t ultrasonic_read(struct file *file,
                               char __user *buf,
                               size_t count,
                               loff_t *ppos)
{
    struct ultrasonic_device *dev = file->private_data;
    unsigned long flags;
    long waited;
    s64 pulse_ns;
    int distance_mm;
    ssize_t ret;

    if (count < sizeof(distance_mm))
        return -EINVAL;

    if (mutex_lock_interruptible(&dev->measure_lock))
        return -ERESTARTSYS;

    if (msleep_interruptible(MEASURE_INTERVAL_MS)) {
        ret = -ERESTARTSYS;
        goto out_unlock;
    }

    if (gpio_get_value(GPIO_ECHO)) {
        ret = -EBUSY;
        goto out_unlock;
    }

    spin_lock_irqsave(&dev->irq_lock, flags);

    reinit_completion(&dev->done);
    dev->rising_seen = false;
    dev->pulse_ns = 0;
    dev->measuring = true;

    spin_unlock_irqrestore(&dev->irq_lock, flags);

    gpio_set_value(GPIO_TRIG, 0);
    udelay(2);
    gpio_set_value(GPIO_TRIG, 1);
    udelay(10);
    gpio_set_value(GPIO_TRIG, 0);

    waited = wait_for_completion_interruptible_timeout(
        &dev->done,
        msecs_to_jiffies(ECHO_TIMEOUT_MS)
    );

    spin_lock_irqsave(&dev->irq_lock, flags);

    dev->measuring = false;
    pulse_ns = dev->pulse_ns;

    spin_unlock_irqrestore(&dev->irq_lock, flags);

    if (waited < 0) {
        ret = waited;
        goto out_unlock;
    }

    if (waited == 0) {
        ret = -ETIMEDOUT;
        goto out_unlock;
    }

    if (pulse_ns <= 0) {
        ret = -EIO;
        goto out_unlock;
    }

    /* Echo 시간(ns) -> 거리(mm) */
    distance_mm = (int)(pulse_ns / 5800);

    if (distance_mm <= 0) {
        ret = -EIO;
        goto out_unlock;
    }

    if (copy_to_user(buf, &distance_mm, sizeof(distance_mm))) {
        ret = -EFAULT;
        goto out_unlock;
    }

    ret = sizeof(distance_mm);

out_unlock:
    mutex_unlock(&dev->measure_lock);
    return ret;
}

static int ultrasonic_release(struct inode *inode,
                              struct file *file)
{
    return 0;
}

static const struct file_operations ultrasonic_fops = {
    .owner   = THIS_MODULE,
    .open    = ultrasonic_open,
    .read    = ultrasonic_read,
    .release = ultrasonic_release,
    .llseek  = no_llseek,
};

/* ---------- 엔코더 기능 ---------- */

struct encoder_sample {
    __s32 steps;
    __u32 pressed;
};

/*
 * 물리 핀 -> GPIO
 * S1: 13번 -> 470
 * S2: 18번 -> 473
 * KEY: 22번 -> 471
 */
static int gpio_s1 = 470;
static int gpio_s2 = 473;
static int gpio_key = 471;

static int transitions = 4;
static bool reverse;

module_param(gpio_s1, int, 0444);
module_param(gpio_s2, int, 0444);
module_param(gpio_key, int, 0444);
module_param(transitions, int, 0444);
module_param(reverse, bool, 0444);

MODULE_PARM_DESC(transitions, "Transitions per detent: 2 or 4");
MODULE_PARM_DESC(reverse, "Reverse rotation direction");

static int irq_s1;
static int irq_s2;
static int previous;
static int partial;
static int steps;

static DEFINE_SPINLOCK(data_lock);
static DEFINE_MUTEX(read_lock);
static atomic_t encoder_opened = ATOMIC_INIT(0);

static int read_ab(void)
{
    return (gpio_get_value(gpio_s1) ? 2 : 0) |
           (gpio_get_value(gpio_s2) ? 1 : 0);
}

static irqreturn_t encoder_irq(int irq, void *data)
{
    static const signed char table[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

    unsigned long flags;
    int ab_state;
    int change;

    spin_lock_irqsave(&data_lock, flags);

    ab_state = read_ab();
    change = table[(previous << 2) | ab_state];

    if ((previous ^ ab_state) == 3)
        partial = 0;

    previous = ab_state;
    partial += change;

    if (partial >= transitions || partial <= -transitions) {
        int direction = partial > 0 ? 1 : -1;

        if (reverse)
            direction = -direction;

        if (steps > -10000 && steps < 10000)
            steps += direction;

        partial = 0;
    }

    spin_unlock_irqrestore(&data_lock, flags);

    return IRQ_HANDLED;
}

static int encoder_open(struct inode *inode, struct file *file)
{
    unsigned long flags;
    int ret;

    if (atomic_cmpxchg(&encoder_opened, 0, 1))
        return -EBUSY;

    spin_lock_irqsave(&data_lock, flags);

    steps = 0;
    partial = 0;
    previous = read_ab();

    spin_unlock_irqrestore(&data_lock, flags);

    ret = nonseekable_open(inode, file);

    if (ret)
        atomic_set(&encoder_opened, 0);

    return ret;
}

static ssize_t encoder_read(struct file *file,
                            char __user *buf,
                            size_t count,
                            loff_t *ppos)
{
    struct encoder_sample sample;
    unsigned long flags;
    ssize_t ret;

    if (count != sizeof(sample))
        return -EINVAL;

    if (mutex_lock_interruptible(&read_lock))
        return -ERESTARTSYS;

    spin_lock_irqsave(&data_lock, flags);
    sample.steps = steps;
    spin_unlock_irqrestore(&data_lock, flags);

    /* KEY는 LOW일 때 눌림 */
    sample.pressed = !gpio_get_value(gpio_key);

    if (copy_to_user(buf, &sample, sizeof(sample))) {
        ret = -EFAULT;
    } else {
        spin_lock_irqsave(&data_lock, flags);

        /* 복사 중 발생한 회전은 다음 read에 보존 */
        steps -= sample.steps;

        spin_unlock_irqrestore(&data_lock, flags);

        ret = sizeof(sample);
    }

    mutex_unlock(&read_lock);

    return ret;
}

static int encoder_release(struct inode *inode, struct file *file)
{
    atomic_set(&encoder_opened, 0);
    return 0;
}

static const struct file_operations encoder_fops = {
    .owner   = THIS_MODULE,
    .open    = encoder_open,
    .read    = encoder_read,
    .release = encoder_release,
    .llseek  = no_llseek,
};

static struct miscdevice encoder_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "fan_encoder",
    .fops  = &encoder_fops,
};

static int encoder_setup(void)
{
    int ret;

    if (transitions != 2 && transitions != 4)
        return -EINVAL;

    ret = gpio_request_one(gpio_s1, GPIOF_IN, "encoder_s1");
    if (ret)
        return ret;

    ret = gpio_request_one(gpio_s2, GPIOF_IN, "encoder_s2");
    if (ret)
        goto free_s1;

    ret = gpio_request_one(gpio_key, GPIOF_IN, "encoder_key");
    if (ret)
        goto free_s2;

    if (gpio_cansleep(gpio_s1) ||
        gpio_cansleep(gpio_s2) ||
        gpio_cansleep(gpio_key)) {
        ret = -EINVAL;
        goto free_key;
    }

    irq_s1 = gpio_to_irq(gpio_s1);
    irq_s2 = gpio_to_irq(gpio_s2);

    if (irq_s1 < 0 || irq_s2 < 0) {
        ret = irq_s1 < 0 ? irq_s1 : irq_s2;
        goto free_key;
    }

    previous = read_ab();

    ret = request_irq(
        irq_s1,
        encoder_irq,
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
        "fan_encoder_s1",
        &encoder_device
    );

    if (ret)
        goto free_key;

    ret = request_irq(
        irq_s2,
        encoder_irq,
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
        "fan_encoder_s2",
        &encoder_device
    );

    if (ret)
        goto free_irq1;

    ret = misc_register(&encoder_device);
    if (ret)
        goto free_irq2;

    pr_info("fan_encoder: ready S1=%d S2=%d KEY=%d\n",
            gpio_s1, gpio_s2, gpio_key);

    return 0;

free_irq2:
    free_irq(irq_s2, &encoder_device);

free_irq1:
    free_irq(irq_s1, &encoder_device);

free_key:
    gpio_free(gpio_key);

free_s2:
    gpio_free(gpio_s2);

free_s1:
    gpio_free(gpio_s1);
    return ret;
}

static void encoder_cleanup(void)
{
    misc_deregister(&encoder_device);

    free_irq(irq_s2, &encoder_device);
    free_irq(irq_s1, &encoder_device);

    gpio_free(gpio_key);
    gpio_free(gpio_s2);
    gpio_free(gpio_s1);
}

/* ---------- 모듈 초기화·종료 ---------- */

static int __init ultrasonic_init(void)
{
    int ret;

    mutex_init(&sensor.measure_lock);
    spin_lock_init(&sensor.irq_lock);
    init_completion(&sensor.done);

    ret = gpio_request(GPIO_TRIG, "ultrasonic_trig");
    if (ret)
        return ret;

    ret = gpio_direction_output(GPIO_TRIG, 0);
    if (ret)
        goto err_trig;

    ret = gpio_request(GPIO_ECHO, "ultrasonic_echo");
    if (ret)
        goto err_trig;

    ret = gpio_direction_input(GPIO_ECHO);
    if (ret)
        goto err_echo;

    if (gpio_cansleep(GPIO_TRIG) || gpio_cansleep(GPIO_ECHO)) {
        ret = -EOPNOTSUPP;
        goto err_echo;
    }

    sensor.irq = gpio_to_irq(GPIO_ECHO);

    if (sensor.irq < 0) {
        ret = sensor.irq;
        goto err_echo;
    }

    ret = request_irq(
        sensor.irq,
        ultrasonic_irq,
        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
        "ultrasonic_echo",
        &sensor
    );

    if (ret)
        goto err_echo;

    sensor.misc.minor = MISC_DYNAMIC_MINOR;
    sensor.misc.name = "ultrasonic";
    sensor.misc.fops = &ultrasonic_fops;

    ret = misc_register(&sensor.misc);
    if (ret)
        goto err_irq;

    ret = encoder_setup();
    if (ret) {
        misc_deregister(&sensor.misc);
        goto err_irq;
    }

    pr_info("ultrasonic: ready Trig=%d Echo=%d IRQ=%d\n",
            GPIO_TRIG, GPIO_ECHO, sensor.irq);

    return 0;

err_irq:
    free_irq(sensor.irq, &sensor);

err_echo:
    gpio_free(GPIO_ECHO);

err_trig:
    gpio_free(GPIO_TRIG);
    return ret;
}

static void __exit ultrasonic_exit(void)
{
    encoder_cleanup();

    misc_deregister(&sensor.misc);
    free_irq(sensor.irq, &sensor);

    gpio_set_value(GPIO_TRIG, 0);
    gpio_free(GPIO_ECHO);
    gpio_free(GPIO_TRIG);

    pr_info("ultrasonic: removed\n");
}

module_init(ultrasonic_init);
module_exit(ultrasonic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Smart Fan ultrasonic and encoder driver");