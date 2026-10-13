#include <linux/module.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

static const int led_gpio[8] = {492, 398, 483, 474, 482, 481, 472, 454};
static const int led_cnt[4]  = {0, 2, 5, 8};   /* 0~3단계 → 켜질 LED 수 */

static void led_show(int level)
{
	int i;

	for (i = 0; i < 8; i++)
		gpio_set_value(led_gpio[i], i < led_cnt[level]);
}

static ssize_t led_write(struct file *f, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	int level;

	if (count != sizeof(level))
		return -EINVAL;
	if (copy_from_user(&level, buf, sizeof(level)))
		return -EFAULT;
	if (level < 0 || level > 3)
		return -EINVAL;
	led_show(level);
	return sizeof(level);
}

static int led_release(struct inode *inode, struct file *f)
{
	led_show(0);			/* 닫으면 전부 꺼짐 */
	return 0;
}

static const struct file_operations led_fops = {
	.owner   = THIS_MODULE,
	.write   = led_write,
	.release = led_release,
};

static struct miscdevice led_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "fan_led",
	.fops  = &led_fops,
};

static int __init led_init(void)
{
	int i, ret;

	for (i = 0; i < 8; i++) {
		ret = gpio_request_one(led_gpio[i], GPIOF_OUT_INIT_LOW, "fan_led");
		if (ret)
			goto err;
	}
	ret = misc_register(&led_dev);
	if (ret)
		goto err;
	return 0;
err:
	while (--i >= 0)
		gpio_free(led_gpio[i]);
	return ret;
}

static void __exit led_exit(void)
{
	int i;

	misc_deregister(&led_dev);
	led_show(0);
	for (i = 0; i < 8; i++)
		gpio_free(led_gpio[i]);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Smart Fan LED bar");
