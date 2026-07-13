#include <linux/module.h>
#include <linux/kernel.h>

static int __init keylogger_init(void)
{
    printk(KERN_INFO "keylogger: module loaded\n");
    return 0;
}

static void __exit keylogger_exit(void)
{
    printk(KERN_INFO "keylogger: module unloaded\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);

MODULE_LICENSE("GPL");