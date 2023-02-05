/* sfl_fs_watch.h v0.1
A filesystem watching utility library for modern Linux and Windows systems

MIT License

Copyright (c) 2023 Michael Dodis

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

REQUIRED LIBRARIES
- Windows
    - shell32.lib
    - Shlwapi.lib
    - Pathcch.lib

COMPILE TIME OPTIONS

#define SFL_FS_WATCH_ZERO_MEMORY(x) memset(&(x), 0, sizeof(x))
    Sets struct memory to zero

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)

@todo:
    - Allow for custom malloc/realloc/free with a lifecycle parameter, i.e.
    for how long is this expected to be used?
    3 values should suffice:
        - As long as the context lives
        - Almost as long as the context lives, but not guaranteed
            - For entries
        - Temporary; will be freed in the same scope it was allocated 
            - For temp string conversions
    - Maybe add filename part to notification struct
*/


#ifndef SFL_FS_WATCH_H
#define SFL_FS_WATCH_H
#include <wchar.h>

#ifdef _WIN32
    #define SFL_FS_WATCH_OS_WINDOWS 1
#endif

#ifdef __linux__
    #define SFL_FS_WATCH_OS_LINUX   1
#endif

#ifndef SFL_FS_WATCH_OS_WINDOWS
    #define SFL_FS_WATCH_OS_WINDOWS 0
#endif

#ifndef SFL_FS_WATCH_OS_LINUX
    #define SFL_FS_WATCH_OS_LINUX   0
#endif

/**
 * The notification types handled
 */
typedef enum {
    SFL_FS_WATCH_NOTIFICATION_INVALID = 0,
    SFL_FS_WATCH_NOTIFICATION_FILE_CREATED = 1,
    SFL_FS_WATCH_NOTIFICATION_FILE_DELETED = 2,
    SFL_FS_WATCH_NOTIFICATION_FILE_MODIFIED = 3
} SflFsWatchNotificationKind;

/** 
 * Generic result.
 * - 0   -> Ok
 * - > 0 -> Warning/Expected
 * - < 0 -> Error
 */
typedef enum {
    /* Function succeeded */
    SFL_FS_WATCH_RESULT_NONE                         = 0,
    /* Function timed out */
    SFL_FS_WATCH_RESULT_TIMEOUT                      = 1,
    /* Function exited because there is nothing to watch anymore */
    SFL_FS_WATCH_RESULT_NO_MORE_DIRECTORIES_TO_WATCH = 2,
    /* Function exited with error */
    SFL_FS_WATCH_RESULT_ERROR                        = -1,
} SflFsWatchResult;

/**
 * Notification, corresponding to watched file/directory
 */
typedef struct {
    /** The file or directory path (absolute) */
    const char *path;
    /** Notification kind, @see SflFsWatchNotificationKind */
    SflFsWatchNotificationKind kind;
    /** The ID of the watched file or directory */
    int id;
} SflFsWatchNotification;

typedef struct _SflFsWatchContext SflFsWatchContext;

/** Event passed to sfl_fs_watch_init() */
#define PROC_SFL_FS_WATCH_NOTIFY(name) void name(SflFsWatchContext *ctx, SflFsWatchNotification *notification, void *usr)
typedef PROC_SFL_FS_WATCH_NOTIFY(ProcSflFsWatchNotify);

/** Used internally. */
typedef struct _SflFsWatchListNode SflFsWatchListNode;
typedef struct _SflFsWatchListNode {
    SflFsWatchListNode *next;
    SflFsWatchListNode *prev;
} SflFsWatchListNode;

#if SFL_FS_WATCH_OS_WINDOWS

/* Declare win32 compatible types to avoid including windows, just in case */
typedef void* SflFsWatchWin32Handle;
typedef struct _SflFsWatchEntry SflFsWatchEntry;

typedef struct _SflFsWatchContext {
    SflFsWatchListNode directories;
    SflFsWatchWin32Handle iocp;
    ProcSflFsWatchNotify *notify_proc;
    void *usr;
    int current_id;
} SflFsWatchContext;

#elif SFL_FS_WATCH_OS_LINUX
typedef struct _SflFsWatchEntry SflFsWatchEntry;

typedef struct _SflFsWatchContext {
    int notify_fd;
    SflFsWatchEntry *entries;    
    ProcSflFsWatchNotify *notify_proc;
    void *usr;
    SflFsWatchListNode directories;
    void *buffer;
    int current_id;
} SflFsWatchContext;

#endif

/**
 * Initializes a context
 * @param ctx    The context, allocated by the user
 * @param notify The notification fucntion pointer. Cannot be zero
 * @param usr    A void pointer passed to the notification function
 */
extern void sfl_fs_watch_init(
    SflFsWatchContext *ctx, 
    ProcSflFsWatchNotify *notify, 
    void *usr);

/**
 * Initializes a context
 * @param ctx    The context, allocated by the user
 * @param notify The notification fucntion pointer. Cannot be zero
 * @param usr    A void pointer passed to the notification function
 */
extern void *sfl_fs_watch_set_usr(SflFsWatchContext *ctx, void *new_usr);

extern void sfl_fs_watch_deinit(SflFsWatchContext *ctx);

/**
 * Add a file or directory to watch
 * @param ctx       The context
 * @param file_path The relative or absolute file to watch
 * @return The ID of the watched path, or negative error in case of failure
 */
extern int sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path);

/**
 * Remove a watched file by ID
 * @param ctx The context
 * @param id  The watch id to remove
 */
extern void sfl_fs_watch_rm_id(SflFsWatchContext *ctx, int id);

/** 
 * Check for any events and handle them if there are any
 * @param ctx The Context
 * @return One of the following:
 *  - SFL_FS_WATCH_RESULT_NONE:    Changes were recorded and handled
 *  - SFL_FS_WATCH_RESULT_TIMEOUT: No changes were recorded
 *  - SFL_FS_WATCH_RESULT_ERROR:   There was an I/O error when polling
 */
extern SflFsWatchResult sfl_fs_watch_poll(SflFsWatchContext *ctx);

/**
 * Wait until at least one event happens
 * @param ctx The context
 */
extern int sfl_fs_watch_wait(SflFsWatchContext *ctx);

/** 
 * Helper to convert a notification enum to a string 
 * @param kind The notification kind
 * @return The string descriptor of the notification kind
 */
static inline const char *sfl_fs_watch_notification_kind_to_string(
    SflFsWatchNotificationKind kind) 
{
    switch (kind) {
        case SFL_FS_WATCH_NOTIFICATION_INVALID:       return "Invalid";  break;
        case SFL_FS_WATCH_NOTIFICATION_FILE_CREATED:  return "Created";  break;
        case SFL_FS_WATCH_NOTIFICATION_FILE_DELETED:  return "Deleted";  break;
        case SFL_FS_WATCH_NOTIFICATION_FILE_MODIFIED: return "Modified"; break;
        default: return ""; break;
    }
}
#endif

#ifdef SFL_FS_WATCH_IMPLEMENTATION

#define SFL_FS_WATCH_MAX_ID 2147483647

#ifndef SFL_FS_WATCH_ZERO_MEMORY
    #include <string.h>
    #define SFL_FS_WATCH_ZERO_MEMORY(x) memset(&(x), 0, sizeof(x))
#endif

#include <stddef.h>

#define SFL_FS_WATCH_CONTAINER_OF(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#include <stdlib.h>
/* Stretchy buffer implementation */
#define sfl_fs_watch_sb_free(x)    ((x) ? free(sfl_fs_watch_sb_raw(x)), 0 : 0)
#define sfl_fs_watch_sb_add(x, n)  (sfl_fs_watch_sb_maybegrow(x, n), sfl_fs_watch_sb_n(x) += (n), &(x)[sfl_fs_watch_sb_n(x)-(n)])
#define sfl_fs_watch_sb_last(x)    ((x)[sfl_fs_watch_sb_n(x)-1])
#define sfl_fs_watch_sb_push(x, v) (sfl_fs_watch_sb_maybegrow(x,1), (x)[sfl_fs_watch_sb_n(x)++] = (v))
#define sfl_fs_watch_sb_count(x)   ((x) ? sfl_fs_watch_sb_n(x) : 0)

#define sfl_fs_watch_sb_raw(x)          ((int *) (void *) (x) - 2)
#define sfl_fs_watch_sb_m(x)            sfl_fs_watch_sb_raw(x)[0]
#define sfl_fs_watch_sb_n(x)            sfl_fs_watch_sb_raw(x)[1]
#define sfl_fs_watch_sb_needsgrow(x, n) ((x) == 0 || sfl_fs_watch_sb_n(x) + (n) >= sfl_fs_watch_sb_m(x))
#define sfl_fs_watch_sb_maybegrow(x, n) (sfl_fs_watch_sb_needsgrow(x,(n)) ? sfl_fs_watch_sb_grow(x, n) : 0)
#define sfl_fs_watch_sb_grow(x, n)      (*((void **)&(x)) = sfl_fs_watch_sb_growf((x), (n), sizeof(*(x))))

static void *sfl_fs_watch_sb_growf(void *arr, int increment, int itemsize)
{
   int dbl_cur = arr ? 2 * sfl_fs_watch_sb_m(arr) : 0;
   int min_needed = sfl_fs_watch_sb_count(arr) + increment;
   int m = dbl_cur > min_needed ? dbl_cur : min_needed;
   int *p = (int *) realloc(arr ? sfl_fs_watch_sb_raw(arr) : 0, itemsize * m + sizeof(int)*2);
   if (p) {
      if (!arr)
         p[1] = 0;
      p[0] = m;
      return p+2;
   } else {
      return (void *) (2*sizeof(int)); // try to force a NULL pointer exception later
   }
}

/* List functions */
static void sfl_fs_watch_list_node_init(SflFsWatchListNode *node);
static int  sfl_fs_watch__list_node_is_empty(SflFsWatchListNode *node);
static void sfl_fs_watch__list_node_unlink(SflFsWatchListNode *node);
static void sfl_fs_watch_list_node_add(
    SflFsWatchListNode *entry, 
    SflFsWatchListNode *prev, 
    SflFsWatchListNode *next);
static void sfl_fs_watch_list_node_append(
    SflFsWatchListNode *node, 
    SflFsWatchListNode *new_node);

/** Flags related to entry's state. Primarily used in directory entries */
typedef enum {
    /** The entry is currently being processed for notifications */
    SFL_FS_WATCH_ENTRY_FLAG_PROCESSING        = 1 << 0,
    /** The entry is pending removal */
    SFL_FS_WATCH_ENTRY_FLAG_REQUESTED_REMOVAL = 1 << 1,
    /** The entry is a directory */
    SFL_FS_WATCH_ENTRY_FLAG_DIRECTORY         = 1 << 2,
} SflFsWatchEntryFlags;

/** Get the if of a new entry and update the current id */
static int sfl_fs_watch_get_next_id(int *current);
/** Returns if the specified entry relates to a folder */
static inline int sfl_fs_watch__entry_is_folder(SflFsWatchEntry *entry);
/** 
 * Returns if the specified entry relates to a folder and has been requested to
 * be watched, by the user 
 */
static inline int sfl_fs_watch__entry_is_watched_folder(SflFsWatchEntry *entry);
/** Find entry by ID, generic version */
static SflFsWatchEntry *sfl_fs_watch__find_entry_by_id(
    SflFsWatchContext *ctx, 
    int id);
/** 
 * Find directory of the file entry. Walks back on the node list until it finds
 * a directory entry 
 */
static SflFsWatchEntry *sfl_fs_watch__find_directory_of_entry(
    SflFsWatchEntry *entry);
/** Remove an entry */
static void sfl_fs_watch__rm_entry(
    SflFsWatchContext *ctx, 
    SflFsWatchEntry *entry);

/** Returns if the node is a file entry and not a directory entry */
static int sfl_fs_watch__entry_is_child_node(SflFsWatchEntry *entry);

#if SFL_FS_WATCH_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <PathCch.h>
#include <shlwapi.h>

typedef struct _SflFsWatchEntry {
    /** The directory handle, if any */
    SflFsWatchWin32Handle handle;
    const wchar_t *file;
    OVERLAPPED *ovl;
    unsigned char *buffer;
    int id;
    SflFsWatchListNode node;
    SflFsWatchListNode dir_node;
    SflFsWatchEntryFlags flags;
} SflFsWatchEntry;

/** Buffer size for notification data */
#define SFL_FS_WATCH_BUFFER_SIZE (64 * 1024 * 1024)

/* Internal Windows functions */

/** Attach file handle to iocp, create iocp if it has not been created yet. */
static void sfl_fs_watch_attach_to_iocp(
    HANDLE file, 
    HANDLE *iocp, 
    ULONG_PTR key, 
    DWORD num_cc_threads);
/** Poll completion port, set timeout = INFINITE for blocking wait */
static DWORD sfl_fs_watch_poll_iocp(
    HANDLE iocp, 
    DWORD timeout, 
    DWORD *bytes_transferred, 
    ULONG_PTR *key, 
    OVERLAPPED **ovl);
/** Issue IO call to ReadDirectoryChangesW for this entry */
static int sfl_fs_watch_issue_entry(SflFsWatchEntry *entry);
/** Duplicate wide string */
static wchar_t *sfl_fs_watch_wstr_dup(const wchar_t *str, int len);
/** Convert string from utf-16 to utf-8 */
static const char *sfl_fs_watch_wstr_to_multibyte(
    const wchar_t *wstr, 
    int wstr_len);
/** Convert string from utf-8 to utf-16 */
static wchar_t *sfl_fs_watch_multibyte_to_wstr(
    const char *multibyte, 
    int multibyte_len, 
    int *wstr_len);
/** Check if wide string ends with specified substring */
static int sfl_fs_watch__wstr_ends_with(
    const wchar_t *str, 
    const wchar_t *end, 
    size_t end_len);
/** Find an entry by filename, wide string version for windows */
static SflFsWatchEntry *sfl_fs_watch__find_entry_by_file(
    SflFsWatchEntry *directory_watch, 
    const wchar_t *filename, 
    size_t filename_len);
/** Modify path to be the directory of the file designated by it */
static int sfl_fs_watch_get_directory_of_file(wchar_t *path, size_t count);
/** Convert relative path to absolute path */
static wchar_t *sfl_fs_watch_convert_relative_to_absolute(
    const wchar_t *path, 
    int path_len, 
    int *result_len);
/**
 * Compare two files in the hierarchy
 * @return
 * 0  If child and directory are the same path,
 * 1  If directory is a complete substring of child
 * -1 Otherwise
 */
static int sfl_fs_watch_compare_files_hierarchy(
    const wchar_t *child, 
    const wchar_t *directory);
/** Poll the IO completion port, set timeout = INFINITE for blocking wait */
static int sfl_fs_watch__poll(SflFsWatchContext *ctx, DWORD timeout);
/** Returns in the specified path is a directory */
static inline int sfl_fs_watch_is_directory(const wchar_t *path);
/** Cancel watch on the specified directory */
static void sfl_fs_watch__cancel_watch(SflFsWatchEntry *directory);
/** Unlink and free directory entry */
static void sfl_fs_watch__delete_directory_entry(SflFsWatchEntry *entry);


static void sfl_fs_watch_init_entry(
    SflFsWatchEntry *entry,
    const wchar_t *file) 
{
    entry->buffer = 0;
    entry->handle = INVALID_HANDLE_VALUE;
    entry->ovl = 0;
    entry->file = file;
    entry->id = -1;
    entry->flags = 0;
    sfl_fs_watch_list_node_init(&entry->node);
    sfl_fs_watch_list_node_init(&entry->dir_node);
}

/**
 * Process notifications for entry
 * @param ctx   The context
 * @param entry The watch entry (always directory)
 * @return 1 if the watch should be re-issued, 0 otherwise
 */
static int sfl_fs_watch__get_notifications(
    SflFsWatchContext *ctx, 
    SflFsWatchEntry *entry) 
{
    const FILE_NOTIFY_INFORMATION *fni = (const FILE_NOTIFY_INFORMATION*)
        entry->buffer;
    for (;;) {
        int id = entry->id;
        wchar_t *filename = 0;
        size_t filename_len = 0;

        if (!sfl_fs_watch__entry_is_watched_folder(entry)) {

            /* 
            Find the file that matches the filename changed. If no such entry
            matches, then we don't report that
            */

            SflFsWatchEntry *file_entry = sfl_fs_watch__find_entry_by_file(
                entry, 
                fni->FileName,
                fni->FileNameLength / sizeof(wchar_t));

            if (file_entry)
                id = file_entry->id;
        }

        {
            size_t a_len = wcslen(entry->file);
            size_t b_len = fni->FileNameLength / sizeof(wchar_t);
            filename_len = a_len + b_len + 1;
            const wchar_t *a = entry->file;
            const wchar_t *b = fni->FileName;
            filename = (wchar_t*)malloc(sizeof(wchar_t) * (filename_len + 1));
            memcpy(filename + 0,         a, sizeof(wchar_t) * a_len);
            filename[a_len] = '\\';
            memcpy(filename + a_len + 1, b, sizeof(wchar_t) * b_len);
            filename[filename_len] = 0;
        }

        const char *path = sfl_fs_watch_wstr_to_multibyte(
            filename,
            filename_len);

        SflFsWatchNotification notification;
        notification.path = path;
        notification.id = id;

        switch (fni->Action) {
            case FILE_ACTION_ADDED:
		    case FILE_ACTION_RENAMED_NEW_NAME:
			    notification.kind = SFL_FS_WATCH_NOTIFICATION_FILE_CREATED;
                break;
		    case FILE_ACTION_REMOVED:
		    case FILE_ACTION_RENAMED_OLD_NAME:
			    notification.kind = SFL_FS_WATCH_NOTIFICATION_FILE_DELETED;
                break;
		    case FILE_ACTION_MODIFIED:
			    notification.kind = SFL_FS_WATCH_NOTIFICATION_FILE_MODIFIED;
                break;
		    default:
			    notification.kind = SFL_FS_WATCH_NOTIFICATION_INVALID;
                break;
        }

        if (notification.id != -1)
            ctx->notify_proc(ctx, &notification, ctx->usr);

        free((void*)path);

        if (entry->flags & SFL_FS_WATCH_ENTRY_FLAG_REQUESTED_REMOVAL) {
            sfl_fs_watch__delete_directory_entry(entry);
            return 0;
        }

        if (!fni->NextEntryOffset)
            break;

        fni = (const FILE_NOTIFY_INFORMATION*)
            ((uintptr_t)(fni) + fni->NextEntryOffset);
    }

    entry->flags &= ~(SFL_FS_WATCH_ENTRY_FLAG_PROCESSING);
    return 1;
}

void sfl_fs_watch_init(
    SflFsWatchContext *ctx, 
    ProcSflFsWatchNotify *notify, 
    void *usr) 
{
    ctx->iocp = 0;
    ctx->notify_proc = notify;
    ctx->usr = usr;
    ctx->current_id = 0;
    sfl_fs_watch_list_node_init(&ctx->directories);
}

void *sfl_fs_watch_set_usr(SflFsWatchContext *ctx, void *new_usr) {
    void *tmp = ctx->usr;
    ctx->usr = new_usr;
    return tmp;
}

int sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path) {

    /* Get parent directory if file_path is a file */
    int file_pathw_len;
    wchar_t *file_pathw = sfl_fs_watch_multibyte_to_wstr(
        file_path, 
        0, 
        &file_pathw_len);

    if (PathIsRelativeW(file_pathw)) {
        const wchar_t *prev_file_pathw = file_pathw;
        int prev_file_pathw_len = file_pathw_len;
        file_pathw = sfl_fs_watch_convert_relative_to_absolute(
            prev_file_pathw,
            file_pathw_len,
            &file_pathw_len);
        free((void*)prev_file_pathw);
    }

    int is_directory = sfl_fs_watch_is_directory(file_pathw);

    SflFsWatchEntry *new_directory = 0;

    /* 
    Search if the file is part of a directory structure we're already 
    watching
    */
    
    for (SflFsWatchListNode *q = ctx->directories.next; 
         q != &ctx->directories;
         q = q->next)
    {

        SflFsWatchEntry *entry = SFL_FS_WATCH_CONTAINER_OF(
            q, 
            SflFsWatchEntry, 
            dir_node);
        
        int hstatus = sfl_fs_watch_compare_files_hierarchy(
            file_pathw,
            entry->file);

        if (hstatus == 1) {

            /* 
            It's a child of a directory that we are already watching
            Add it to the list and make sure that the entry will recurse
            in the directory's substructure
            */
            SflFsWatchEntry* new_entry = (SflFsWatchEntry*)
                malloc(sizeof(SflFsWatchEntry));

            sfl_fs_watch_init_entry(new_entry, file_pathw);
            new_entry->id = sfl_fs_watch_get_next_id(&ctx->current_id);

            sfl_fs_watch_list_node_append(&entry->node, &new_entry->node);
            return new_entry->id;
        } else if (hstatus == 0) {
            if (entry->id == -1) {
                entry->id = sfl_fs_watch_get_next_id(&ctx->current_id);
            }
            return entry->id;
        }
    }

    int ret_id = -1;

    /* If we didn't find a node, then create one */
    if (!is_directory) {
        SflFsWatchEntry *new_entry = malloc(sizeof(SflFsWatchEntry));
        new_directory = malloc(sizeof(SflFsWatchEntry));
        
        sfl_fs_watch_init_entry(new_entry, sfl_fs_watch_wstr_dup(
            file_pathw, 
            file_pathw_len));

        new_entry->id = sfl_fs_watch_get_next_id(&ctx->current_id);
        
        sfl_fs_watch_get_directory_of_file(file_pathw, file_pathw_len);
        sfl_fs_watch_init_entry(new_directory, file_pathw);

        sfl_fs_watch_list_node_append(&new_directory->node, &new_entry->node);
        ret_id = new_entry->id;
    } else {
        new_directory = malloc(sizeof(SflFsWatchEntry));
        sfl_fs_watch_init_entry(new_directory, file_pathw);
        new_directory->id = sfl_fs_watch_get_next_id(&ctx->current_id);
        ret_id = new_directory->id;
    }

    /* Open directory handle */
    HANDLE dir_handle = CreateFileW(
        new_directory->file,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    /* Associate handle entry with io completion port */
    OVERLAPPED *ovl = (OVERLAPPED*)calloc(1, sizeof(OVERLAPPED));
    ovl->hEvent = CreateEvent(0, TRUE, FALSE, 0);

    new_directory->handle = dir_handle;
    new_directory->buffer = malloc(SFL_FS_WATCH_BUFFER_SIZE);
    new_directory->ovl = ovl;
    sfl_fs_watch_attach_to_iocp(
        new_directory->handle, 
        &ctx->iocp, 
        (ULONG_PTR)new_directory, 
        0);

    sfl_fs_watch_list_node_append(&ctx->directories, &new_directory->dir_node);
    sfl_fs_watch_issue_entry(new_directory);

    return ret_id;
}

void sfl_fs_watch_rm_id(SflFsWatchContext *ctx, int id) {
    SflFsWatchEntry *entry = sfl_fs_watch__find_entry_by_id(ctx, id);
    if (!entry)
        return;

    sfl_fs_watch__rm_entry(ctx, entry);
}

int sfl_fs_watch_poll(SflFsWatchContext *ctx) {
    return sfl_fs_watch__poll(ctx, 0);
}

int sfl_fs_watch_wait(SflFsWatchContext *ctx) {
    return sfl_fs_watch__poll(ctx, INFINITE);
}

void sfl_fs_watch_deinit(SflFsWatchContext *ctx) {

}


/* Internal Windows function implementations */

static void sfl_fs_watch__rm_entry(
    SflFsWatchContext *ctx, 
    SflFsWatchEntry *entry)
{
    SflFsWatchEntry *directory = sfl_fs_watch__find_directory_of_entry(entry);

    if (sfl_fs_watch__entry_is_child_node(entry)) {
        sfl_fs_watch__list_node_unlink(&entry->node);

        free(entry);
    } else {
        /* If it's just a directory, then release the id */
        entry->id = -1;
    }

    /* 
    if it is a watched directory, then remove it or mark it for deletion if it's
    being processed
    */
    if (sfl_fs_watch__list_node_is_empty(&directory->node) && 
        !sfl_fs_watch__entry_is_watched_folder(directory)) 
    {
        if (!(directory->flags & SFL_FS_WATCH_ENTRY_FLAG_PROCESSING)) {
            sfl_fs_watch__delete_directory_entry(directory);
        } else {
            directory->flags |= SFL_FS_WATCH_ENTRY_FLAG_REQUESTED_REMOVAL;
        }
    }
}

static void sfl_fs_watch__cancel_watch(SflFsWatchEntry *directory) {
    CancelIo(directory->handle);
    DWORD ret = WaitForSingleObject(directory->ovl->hEvent, 1000);
    CloseHandle(directory->ovl->hEvent);

    if (ret == WAIT_OBJECT_0 || GetLastError() == ERROR_OPERATION_ABORTED) {
        SetLastError(0);
    } else {
        /* @todo: This could be terrible. At least report an error here */
    }
}

static void sfl_fs_watch_attach_to_iocp(
    HANDLE file, 
    HANDLE *iocp,
    ULONG_PTR key,
    DWORD num_cc_threads)
{
    HANDLE tmp_iocp = *iocp;
    *iocp = CreateIoCompletionPort(file, tmp_iocp, key, num_cc_threads);
}

static DWORD sfl_fs_watch_poll_iocp(
    HANDLE iocp,
    DWORD timeout,
    DWORD *bytes_transferred,
    ULONG_PTR *key,
    OVERLAPPED **ovl)
{
    if (iocp == 0) {
        /* Invalid handle */
        return -1;
    }

    *bytes_transferred = 0;
    *key = 0;
    *ovl = 0;

    if (GetQueuedCompletionStatus(iocp, bytes_transferred, key, ovl, timeout))
        return 0;
    DWORD err = GetLastError();
    if ((err == WAIT_TIMEOUT) || (err == ERROR_OPERATION_ABORTED)) {
        SetLastError(0);
    }

    return err;
}

static int sfl_fs_watch_issue_entry(SflFsWatchEntry *entry) {
    if (entry->handle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    HANDLE dir_handle = (HANDLE)entry->handle;

    const DWORD filter = 
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION;

    DWORD undefined = 0;
    ReadDirectoryChangesW(
        dir_handle,
        (LPVOID)entry->buffer,
        (DWORD)SFL_FS_WATCH_BUFFER_SIZE,
        FALSE,
        filter,
        &undefined,
        (OVERLAPPED*)entry->ovl,
        0);
    return 0;
}

static wchar_t *sfl_fs_watch_wstr_dup(const wchar_t *str, int len) {
    wchar_t *result = malloc(sizeof(wchar_t) * (len + 1));
    for (int i = 0; i < len; ++i) {
        result[i] = str[i];
    }
    result[len] = 0;
    return result;
}

static const char *sfl_fs_watch_wstr_to_multibyte(
    const wchar_t *wstr,
    int wstr_len) 
{
    if (wstr_len == 0)
        wstr_len = (int)wcslen(wstr);

    int num_bytes = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wstr,
        wstr_len,
        0, 0, 0, 0);
    
    char *result = malloc(num_bytes + 1);

    WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        wstr,
        wstr_len,
        result,
        num_bytes,
        0,
        0);

    result[num_bytes] = 0;
    return result;    
}

static wchar_t *sfl_fs_watch_multibyte_to_wstr(
    const char *multibyte,
    int multibyte_len,
    int *wstr_len)
{
    if (multibyte_len == 0)
        multibyte_len = (int)strlen(multibyte);

    *wstr_len = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        multibyte,
        multibyte_len,
        0,
        0);

    wchar_t *result = malloc(sizeof(wchar_t) * (*wstr_len + 1));
    
    MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        multibyte,
        multibyte_len,
        result,
        *wstr_len);

    result[*wstr_len] = 0;
    return result;
}

static int sfl_fs_watch__wstr_ends_with(
    const wchar_t *str, 
    const wchar_t *end,
    size_t end_len)
{
    const wchar_t *s = str;
    const wchar_t *e = end + end_len;
    
    /* Move to end */
    while (*s++ != 0);
    
    s -= 2;
    e--;


    /* Check backwards */
    while (*e == *s) {
        if (e == end) {
            return 1;
        }

        if (s == end) {
            return 0;
        }

        e--;
        s--;
    }

    return 0;
}

static SflFsWatchEntry *sfl_fs_watch__find_entry_by_file(
    SflFsWatchEntry *directory_watch,
    const wchar_t *filename,
    size_t filename_len)
{
    for (SflFsWatchListNode *node = directory_watch->node.next;
         node != &directory_watch->node;
         node = node->next)
    {
        SflFsWatchEntry *entry = SFL_FS_WATCH_CONTAINER_OF(
            node, 
            SflFsWatchEntry, 
            node);

        if (sfl_fs_watch__wstr_ends_with(
            entry->file, 
            filename, 
            filename_len)) 
        {
            return entry;
        }
    }

    return 0;
}

static int sfl_fs_watch_get_directory_of_file(wchar_t *path, size_t count) {
    HRESULT res = PathCchRemoveFileSpec(path, count);
    return res == S_OK ? 1 : 0;
}

static wchar_t *sfl_fs_watch_convert_relative_to_absolute(
    const wchar_t *path,
    int path_len,
    int *result_len)
{
    if (path_len == 0) {
        path_len = (int)wcslen(path);
    }
    *result_len = GetFullPathNameW(
        path,
        0,
        0, 
        0);
    
    wchar_t *result = malloc(sizeof(wchar_t) * (*result_len));

    GetFullPathNameW(
        path,
        *result_len,
        result,
        0);
    
    *result_len -= 1;
    return result;
}

static int sfl_fs_watch_compare_files_hierarchy(
    const wchar_t *child, 
    const wchar_t *directory)
{
    size_t directory_len = wcslen(directory);
    size_t child_len = wcslen(child);
    if (directory_len > child_len) {
        return -1;
    }

    for (int i = 0; i < directory_len; ++i) {
        if (directory[i] != child[i]) {
            return -1;
        }
    }

    return (directory_len == child_len) ? 0 : 1;
}


static int sfl_fs_watch__poll(SflFsWatchContext *ctx, DWORD timeout) {

    if (sfl_fs_watch__list_node_is_empty(&ctx->directories)) {
        return SFL_FS_WATCH_RESULT_NO_MORE_DIRECTORIES_TO_WATCH;
    }

POLL_AGAIN:
    DWORD bytes_transferred;
    ULONG_PTR key;
    OVERLAPPED *ovl;
    
    DWORD result = sfl_fs_watch_poll_iocp(
        ctx->iocp, 
        timeout,
        &bytes_transferred,
        &key,
        &ovl);

    if (result == ERROR_OPERATION_ABORTED) {
        goto POLL_AGAIN;
    }

    if (result != ERROR_SUCCESS) {
        return result;
    }

    SflFsWatchEntry *entry = (SflFsWatchEntry*)key;
    entry->flags |= SFL_FS_WATCH_ENTRY_FLAG_PROCESSING;

    int resume = sfl_fs_watch__get_notifications(ctx, entry);
    if (resume) {
        int err = sfl_fs_watch_issue_entry(entry);
        return err;
    }
    return SFL_FS_WATCH_RESULT_NONE;
}

static inline int sfl_fs_watch_is_directory(const wchar_t *path) {
    DWORD attributes = GetFileAttributesW(path);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        return -1;
    }

    return (attributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
}

static void sfl_fs_watch__delete_directory_entry(SflFsWatchEntry *entry) {
    sfl_fs_watch__list_node_unlink(&entry->dir_node);
    sfl_fs_watch__cancel_watch(entry);

    free(entry->ovl);
    free(entry->buffer);
    free(entry);
}


#elif SFL_FS_WATCH_OS_LINUX
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <libgen.h>

typedef struct _SflFsWatchEntry {
    const char *file;
    /** Watch id generated by inotify */
    int handle;
    /** Watch id generated by library */
    int id;
    SflFsWatchEntryFlags flags;
    SflFsWatchListNode node;
    SflFsWatchListNode dir_node;
} SflFsWatchEntry;

#define SFL_FS_WATCH_BUFFER_SIZE (sizeof(struct inotify_event) + NAME_MAX + 1)

/* Internal Linux functions */
static void sfl_fs_watch_init_entry(
    SflFsWatchEntry *entry,
    const char *file);
static int sfl_fs_watch_is_directory(const char *path);
static char *sfl_fs_watch_str_dup(const char *str);
static void sfl_fs_watch_get_directory_of_file(char *path);
static int sfl_fs_watch_compare_files_hierarchy(
    const char *child,
    const char *directory);

/** Cancel watch on the specified directory */
static void sfl_fs_watch__cancel_watch(
    SflFsWatchContext *ctx, 
    SflFsWatchEntry *directory);

static int sfl_fs_watch__poll(SflFsWatchContext *ctx);
static void sfl_fs_watch__delete_directory_entry(
    SflFsWatchContext *ctx,
    SflFsWatchEntry *entry);

static SflFsWatchEntry *sfl_fs_watch__find_entry_by_file(
    SflFsWatchEntry *directory,
    const char *file);

static int sfl_fs_watch__str_ends_with(const char *str, const char *end);

static char *sfl_fs_watch__join_path(const char *a, const char *b);

void sfl_fs_watch_init(
    SflFsWatchContext *ctx, 
    ProcSflFsWatchNotify *notify, 
    void *usr) 
{
    ctx->notify_fd = inotify_init1(IN_NONBLOCK);
    ctx->notify_proc = notify;
    ctx->usr = usr;
    ctx->buffer = malloc(SFL_FS_WATCH_BUFFER_SIZE);
    ctx->current_id = 0;
    sfl_fs_watch_list_node_init(&ctx->directories);
}

void *sfl_fs_watch_set_usr(SflFsWatchContext *ctx, void *new_usr) {
    void *tmp = ctx->usr;
    ctx->usr = new_usr;
    return tmp;
}

int sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path) {

    char *absolute_path = (char*)malloc(PATH_MAX + 1);
    absolute_path[PATH_MAX] = 0;

    if (!realpath(file_path, absolute_path)) {
        return -1;
    }

    int is_directory = sfl_fs_watch_is_directory(absolute_path);

    SflFsWatchEntry *new_directory = 0;

    /* 
    Search if the file is part of a directory structure we're already 
    watching
    */
    for (SflFsWatchListNode *q = ctx->directories.next; 
         q != &ctx->directories;
         q = q->next)
    {

        SflFsWatchEntry *entry = SFL_FS_WATCH_CONTAINER_OF(
            q, 
            SflFsWatchEntry, 
            dir_node);
        
        int hstatus = sfl_fs_watch_compare_files_hierarchy(
            absolute_path,
            entry->file);

        if (hstatus == 1) {

            /* 
            It's a child of a directory that we are already watching
            Add it to the list and make sure that the entry will recurse
            in the directory's substructure
            */
            SflFsWatchEntry* new_entry = (SflFsWatchEntry*)
                malloc(sizeof(SflFsWatchEntry));

            sfl_fs_watch_init_entry(new_entry, absolute_path);
            new_entry->id = sfl_fs_watch_get_next_id(&ctx->current_id);

            sfl_fs_watch_list_node_append(&entry->node, &new_entry->node);
            return new_entry->id;
        } else if (hstatus == 0) {
            if (entry->id == -1) {
                entry->id = sfl_fs_watch_get_next_id(&ctx->current_id);
            }
            return entry->id;
        }
    }

    int ret_id = -1;

    /* If we didn't find a node, then create one */
    if (!is_directory) {
        SflFsWatchEntry *new_entry = malloc(sizeof(SflFsWatchEntry));
        new_directory = malloc(sizeof(SflFsWatchEntry));
        
        sfl_fs_watch_init_entry(
            new_entry, 
            sfl_fs_watch_str_dup(absolute_path));

        new_entry->id = sfl_fs_watch_get_next_id(&ctx->current_id);
        
        sfl_fs_watch_get_directory_of_file(absolute_path);
        sfl_fs_watch_init_entry(new_directory, absolute_path);

        sfl_fs_watch_list_node_append(&new_directory->node, &new_entry->node);
        ret_id = new_entry->id;
    } else {
        new_directory = malloc(sizeof(SflFsWatchEntry));
        sfl_fs_watch_init_entry(new_directory, absolute_path);
        new_directory->id = sfl_fs_watch_get_next_id(&ctx->current_id);
        ret_id = new_directory->id;
    }

    int fd = inotify_add_watch(
        ctx->notify_fd, 
        new_directory->file, 
        IN_CREATE | IN_DELETE | IN_MODIFY);

    if (fd == -1) {
        return -1;
    }

    new_directory->handle = fd;
    new_directory->flags |= SFL_FS_WATCH_ENTRY_FLAG_DIRECTORY;
    sfl_fs_watch_list_node_append(&ctx->directories, &new_directory->dir_node);

    return ret_id;
}

void sfl_fs_watch_rm_id(SflFsWatchContext *ctx, int id) {
    SflFsWatchEntry *entry = sfl_fs_watch__find_entry_by_id(ctx, id);
    if (!entry)
        return;

    sfl_fs_watch__rm_entry(ctx, entry);
}

int sfl_fs_watch_poll(SflFsWatchContext *ctx) {
    int fl = fcntl(ctx->notify_fd, F_GETFL);
    fl |= O_NONBLOCK;
    fcntl(ctx->notify_fd, F_SETFL, fl);
    return sfl_fs_watch__poll(ctx);
}

int sfl_fs_watch_wait(SflFsWatchContext *ctx) {
    int fl = fcntl(ctx->notify_fd, F_GETFL);
    fl &= ~(O_NONBLOCK);
    fcntl(ctx->notify_fd, F_SETFL, fl);
    return sfl_fs_watch__poll(ctx);
}

/* Internal Linux function implementations */

static void sfl_fs_watch_init_entry(
    SflFsWatchEntry *entry,
    const char *file) 
{
    entry->file = file;
    entry->id = -1;
    entry->flags = 0;
    sfl_fs_watch_list_node_init(&entry->node);
    sfl_fs_watch_list_node_init(&entry->dir_node);
}

static int sfl_fs_watch_is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf)) {
        return -1;
    }

    return S_ISDIR(statbuf.st_mode);
}

static char *sfl_fs_watch_str_dup(const char *str) {
    size_t len = strlen(str);
    char *result = (char*)malloc(len + 1);
    for (int i = 0; i < len; ++i) {
        result[i] = str[i];
    }
    result[len] = 0;
    return result;
}

static void sfl_fs_watch_get_directory_of_file(char *path) {
    dirname(path);
}

static int sfl_fs_watch_compare_files_hierarchy(
    const char *child,
    const char *directory)
{
    size_t directory_len = strlen(directory);
    size_t child_len = strlen(child);
    if (directory_len > child_len) {
        return -1;
    }

    for (int i = 0; i < directory_len; ++i) {
        if (directory[i] != child[i]) {
            return -1;
        }
    }

    return (directory_len == child_len) ? 0 : 1;
}

static void sfl_fs_watch__rm_entry(
    SflFsWatchContext *ctx, 
    SflFsWatchEntry *entry)
{
    SflFsWatchEntry *directory = sfl_fs_watch__find_directory_of_entry(entry);

    if (sfl_fs_watch__entry_is_child_node(entry)) {
        sfl_fs_watch__list_node_unlink(&entry->node);

        free(entry);
    } else {
        /* If it's just a directory, then release the id */
        entry->id = -1;
    }

    /* 
    if it is a watched directory, then remove it or mark it for deletion if it's
    being processed
    */
    if (sfl_fs_watch__list_node_is_empty(&directory->node) && 
        !sfl_fs_watch__entry_is_watched_folder(directory)) 
    {
        if (!(directory->flags & SFL_FS_WATCH_ENTRY_FLAG_PROCESSING)) {
            sfl_fs_watch__delete_directory_entry(ctx, directory);
        } else {
            directory->flags |= SFL_FS_WATCH_ENTRY_FLAG_REQUESTED_REMOVAL;
        }
    }
}

static int sfl_fs_watch__get_notifications(
    SflFsWatchContext *ctx,
    struct inotify_event *event)
{

    /* Find directory of watch file descriptor */        
    SflFsWatchEntry *directory = 0;

    for (SflFsWatchListNode *n = ctx->directories.next;
            n != &ctx->directories;
            n = n->next)
    {
        SflFsWatchEntry *ne = SFL_FS_WATCH_CONTAINER_OF(
            n,
            SflFsWatchEntry,
            dir_node);

        if (ne->handle != event->wd)
            continue;

        directory = ne;
        break;
    }

    if (!directory) {
        return 0;
    }

    int id = directory->id;
    const char *filename = event->name;
    int free_filename = 0;    
    if (!sfl_fs_watch__entry_is_watched_folder(directory)) {
        SflFsWatchEntry *file_entry = sfl_fs_watch__find_entry_by_file(
            directory,
            event->name);
        
        if (file_entry) {
            id = file_entry->id;
            filename = file_entry->file;
        }
    } else {
        filename = sfl_fs_watch__join_path(directory->file, event->name);
        free_filename = 1;
    }

    SflFsWatchNotification notification;
    notification.path = filename;
    notification.id = id;
    notification.kind = SFL_FS_WATCH_NOTIFICATION_FILE_MODIFIED;

    directory->flags |= SFL_FS_WATCH_ENTRY_FLAG_PROCESSING;

    if (notification.id != -1)
        ctx->notify_proc(ctx, &notification, ctx->usr);
    
    if (directory->flags & SFL_FS_WATCH_ENTRY_FLAG_REQUESTED_REMOVAL) {
        sfl_fs_watch__delete_directory_entry(ctx, directory);
    }

    directory->flags &= ~(SFL_FS_WATCH_ENTRY_FLAG_PROCESSING);

    if (free_filename)
        free(filename);

    return 1;
}

static int sfl_fs_watch__poll(SflFsWatchContext *ctx) {
    if (sfl_fs_watch__list_node_is_empty(&ctx->directories)) {
        return SFL_FS_WATCH_RESULT_NO_MORE_DIRECTORIES_TO_WATCH;
    }

    unsigned char *buffer = (unsigned char*)ctx->buffer;
    int length;
    int count = 0;

    for (;;) {
        length = read(ctx->notify_fd, buffer, SFL_FS_WATCH_BUFFER_SIZE);

        if (length == 0) {
            break;
        } else if (length < 0) {
            if (errno == EAGAIN) {
                return count > 0 
                    ? SFL_FS_WATCH_RESULT_NONE 
                    : SFL_FS_WATCH_RESULT_TIMEOUT;
            } else {
                return SFL_FS_WATCH_RESULT_ERROR;
            }
        }

        int offset = 0;
        while ((offset + 1) < length) {
            struct inotify_event *event = (struct inotify_event*)
                buffer + offset;
            
            sfl_fs_watch__get_notifications(ctx, event);

            if (sfl_fs_watch__list_node_is_empty(&ctx->directories)) {
                return SFL_FS_WATCH_RESULT_NO_MORE_DIRECTORIES_TO_WATCH;
            }

            /* 
            @note: -1 because event->len obviously includes the char itself 
            */
            offset += sizeof(struct inotify_event) + event->len - 1;
        }

        count++;
    }

    return SFL_FS_WATCH_RESULT_NONE;
}

static void sfl_fs_watch__delete_directory_entry(
    SflFsWatchContext *ctx,
    SflFsWatchEntry *entry) 
{
    sfl_fs_watch__list_node_unlink(&entry->dir_node);
    sfl_fs_watch__cancel_watch(ctx, entry);

    free((void*)entry->file);
    free(entry);
}

static void sfl_fs_watch__cancel_watch(
    SflFsWatchContext *ctx,
    SflFsWatchEntry *directory) 
{
    inotify_rm_watch(ctx->notify_fd, directory->handle);
}

static SflFsWatchEntry *sfl_fs_watch__find_entry_by_file(
    SflFsWatchEntry *directory,
    const char *file)
{
    for (SflFsWatchListNode *node = directory->node.next;
         node != &directory->node;
         node = node->next)
    {
        SflFsWatchEntry *entry = SFL_FS_WATCH_CONTAINER_OF(
            node, 
            SflFsWatchEntry, 
            node);

        if (sfl_fs_watch__str_ends_with(entry->file, file)) {
            return entry;
        }
    }

    return 0;  
}

static int sfl_fs_watch__str_ends_with(const char *str, const char *end) {
    const char *s = str;
    const char *e = end + strlen(end);
    
    /* Move to end */
    while (*s++ != 0);
    
    s -= 2;
    e--;


    /* Check backwards */
    while (*e == *s) {
        if (e == end) {
            return 1;
        }

        if (s == end) {
            return 0;
        }

        e--;
        s--;
    }

    return 0;
}

static char *sfl_fs_watch__join_path(const char *a, const char *b) {
    size_t a_len = strlen(a);
    size_t b_len = strlen(b);

    size_t final_size = a_len + b_len + 1;
    char *result = malloc(final_size + 1);
    memcpy(result + 0,         a, a_len);
    result[a_len] = '/';
    memcpy(result + a_len + 1, b, b_len);
    result[final_size] = 0;

    return result;
}

/* Linux */
#endif

/* Generic implementations */
static void sfl_fs_watch_list_node_init(SflFsWatchListNode *node) {
    node->next = node;
    node->prev = node;
}

static void sfl_fs_watch_list_node_add(
    SflFsWatchListNode *entry,
    SflFsWatchListNode *prev,
    SflFsWatchListNode *next)
{
    next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

static void sfl_fs_watch_list_node_append(
    SflFsWatchListNode *node, 
    SflFsWatchListNode *new_node)
{
    sfl_fs_watch_list_node_add(new_node, node->prev, node);
}

static int sfl_fs_watch__list_node_is_empty(SflFsWatchListNode *node) {
    return (node->next == node) && (node->prev == node);
}

static void sfl_fs_watch__list_node_unlink(SflFsWatchListNode *node) {
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

static int sfl_fs_watch_get_next_id(int *current) {
    if (*current == SFL_FS_WATCH_MAX_ID) {
        *current = 0;
    }
    return (*current)++;
}

static inline int sfl_fs_watch__entry_is_folder(SflFsWatchEntry *entry) {
    return (entry->flags & SFL_FS_WATCH_ENTRY_FLAG_DIRECTORY);
}

static inline int sfl_fs_watch__entry_is_watched_folder(
    SflFsWatchEntry *entry)
{
    return (entry->id != -1) && sfl_fs_watch__entry_is_folder(entry);
}

static SflFsWatchEntry *sfl_fs_watch__find_entry_by_id(
    SflFsWatchContext *ctx, 
    int id) 
{
    for (SflFsWatchListNode *dn = ctx->directories.next;
         dn != &ctx->directories;
         dn = dn->next)
    {
        SflFsWatchEntry *directory_entry = SFL_FS_WATCH_CONTAINER_OF(
            dn, 
            SflFsWatchEntry, 
            dir_node);
        
        if (directory_entry->id == id) {
            return directory_entry;
        }

        for (SflFsWatchListNode *fn = directory_entry->node.next;
             fn != &directory_entry->node;
             fn = fn->next)
        {
            SflFsWatchEntry *file_entry = SFL_FS_WATCH_CONTAINER_OF(
                fn, 
                SflFsWatchEntry, 
                node);
            if (file_entry->id == id) {
                return file_entry;
            }
        }
    }

    return 0;
}

static SflFsWatchEntry *sfl_fs_watch__find_directory_of_entry(
    SflFsWatchEntry *entry) 
{
    SflFsWatchListNode *q = &entry->node;
    SflFsWatchEntry *qn = entry;
    while (!sfl_fs_watch__entry_is_folder(qn)) {
        q = q->prev;
        qn = SFL_FS_WATCH_CONTAINER_OF(q, SflFsWatchEntry, node);
    }

    return qn;
}

static int sfl_fs_watch__entry_is_child_node(SflFsWatchEntry *entry) {
    return sfl_fs_watch__list_node_is_empty(&entry->dir_node);
}


#endif

