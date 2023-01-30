/*
===SFL FS Watch===
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

COMPILE TIME OPTIONS

#define SFL_FS_WATCH_ZERO_MEMORY(x) memset(&(x), 0, sizeof(x))
    Sets struct memory to zero

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)

@todo:
    - Allow for custom malloc/realloc/free
*/

#ifndef SFL_FS_WATCH_H
#define SFL_FS_WATCH_H

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

typedef enum {
    SFL_FS_WATCH_NOTIFICATION_INVALID = 0,
    SFL_FS_WATCH_NOTIFICATION_FILE_CREATED = 1,
    SFL_FS_WATCH_NOTIFICATION_FILE_DELETED = 2,
    SFL_FS_WATCH_NOTIFICATION_FILE_MODIFIED = 3
} SflFsWatchNotificationKind;

typedef enum {
    SFL_FS_WATCH_RESULT_NONE = 0,
    SFL_FS_WATCH_RESULT_TIMEOUT = 1,
    SFL_FS_WATCH_RESULT_ERROR = -1,
} SflFsWatchResult;

typedef struct {
    const char *path;
    /** Notification kind, @see SflFsWatchNotificationKind */
    SflFsWatchNotificationKind kind;
} SflFsWatchFileNotification;

#define PROC_SFL_FS_WATCH_NOTIFY(name) void name(SflFsWatchFileNotification *notification, void *usr)
typedef PROC_SFL_FS_WATCH_NOTIFY(ProcSflFsWatchNotify);

#if SFL_FS_WATCH_OS_WINDOWS
/* Declare win32 compatible types to avoid including windows, just in case */
typedef void* SflFsWatchWin32Handle;
typedef struct _SflFsWatchEntry SflFsWatchEntry;

typedef struct {
    SflFsWatchEntry *entries;
    const char **watched_files;
    SflFsWatchWin32Handle iocp;
    ProcSflFsWatchNotify *notify_proc;
    void *usr;
} SflFsWatchContext;

#elif SFL_FS_WATCH_OS_LINUX
typedef struct _SflFsWatchEntry SflFsWatchEntry;

typedef struct {
    int notify_fd;
    SflFsWatchEntry *entries;    
    ProcSflFsWatchNotify *notify_proc;
    void *usr;
    void *buffer;
} SflFsWatchContext;

#endif


extern void sfl_fs_watch_init(SflFsWatchContext *ctx, ProcSflFsWatchNotify *notify, void *usr);
extern void sfl_fs_watch_deinit(SflFsWatchContext *ctx);
extern int  sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path);
/** 
 * Check for any events and handle them if there are any
 * @param ctx The Context
 * @return One of the following:
 *  - SFL_FS_WATCH_RESULT_NONE:    Changes were recorded and handled
 *  - SFL_FS_WATCH_RESULT_TIMEOUT: No changes were recorded
 *  - SFL_FS_WATCH_RESULT_ERROR:   There was an I/O error when polling
 */
extern SflFsWatchResult sfl_fs_watch_poll(SflFsWatchContext *ctx);
extern int  sfl_fs_watch_wait(SflFsWatchContext *ctx);
static inline const char *sfl_fs_watch_notification_kind_to_string(SflFsWatchNotificationKind kind) {
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

#ifndef SFL_FS_WATCH_ZERO_MEMORY
    #include <string.h>
    #define SFL_FS_WATCH_ZERO_MEMORY(x) memset(&(x), 0, sizeof(x))
#endif

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

#if SFL_FS_WATCH_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct _SflFsWatchEntry {
    SflFsWatchWin32Handle handle;
    const char *directory;
    OVERLAPPED *ovl;
    unsigned char *buffer;
} SflFsWatchEntry;


#define SFL_FS_WATCH_BUFFER_SIZE (64 * 1024 * 1024)

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

static void sfl_fs_watch_get_notifications(
    SflFsWatchContext *ctx, 
    SflFsWatchEntry *entry) 
{
    const FILE_NOTIFY_INFORMATION *fni = (const FILE_NOTIFY_INFORMATION*)
        entry->buffer;
    for (;;) {
        const char *path = sfl_fs_watch_wstr_to_multibyte(
            fni->FileName,
            fni->FileNameLength / sizeof(wchar_t));

        SflFsWatchFileNotification notification;
        notification.path = path;

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

        ctx->notify_proc(&notification, ctx->usr);

        if (!fni->NextEntryOffset)
            break;

        fni = (const FILE_NOTIFY_INFORMATION*)
            ((uintptr_t)(fni) + fni->NextEntryOffset);
    }
}

void sfl_fs_watch_init(SflFsWatchContext *ctx, ProcSflFsWatchNotify *notify, void *usr) {
    ctx->entries = 0;
    ctx->watched_files = 0;
    ctx->iocp = 0;
    ctx->notify_proc = notify;
    ctx->usr = usr;
}

/* @todo: for now this only accepts directories */
void sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path) {
    /* Add file or directory */
    sfl_fs_watch_sb_push(ctx->watched_files, file_path);

    /* Open directory handle */
    HANDLE dir_handle = CreateFileA(
        file_path,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    /* Associate handle entry with io completion port */
    OVERLAPPED *ovl = (OVERLAPPED*)calloc(1, sizeof(OVERLAPPED));
    ovl->hEvent = CreateEvent(0, TRUE, FALSE, 0);
    
    {
        SflFsWatchEntry entry;
        entry.handle = dir_handle;
        entry.directory = file_path;
        entry.buffer = (unsigned char*)malloc(SFL_FS_WATCH_BUFFER_SIZE);
        entry.ovl = (OVERLAPPED*)calloc(1, sizeof(OVERLAPPED));
        entry.ovl->hEvent = CreateEvent(0, TRUE, FALSE, 0);
        sfl_fs_watch_sb_push(ctx->entries, entry);
    }

    SflFsWatchEntry *entry = &sfl_fs_watch_sb_last(ctx->entries);
    sfl_fs_watch_attach_to_iocp(entry->handle, &ctx->iocp, (ULONG_PTR)entry, 0);

    sfl_fs_watch_issue_entry(entry);
}


static int sfl__fs_watch_poll(SflFsWatchContext *ctx, DWORD timeout) {
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
    sfl_fs_watch_get_notifications(ctx, entry);
    int err = sfl_fs_watch_issue_entry(entry);
    return err;
}

int sfl_fs_watch_poll(SflFsWatchContext *ctx) {
    return sfl__fs_watch_poll(ctx, 0);
}

int sfl_fs_watch_wait(SflFsWatchContext *ctx) {
    return sfl__fs_watch_poll(ctx, INFINITE);
}

void sfl_fs_watch_deinit(SflFsWatchContext *ctx) {

}

#elif SFL_FS_WATCH_OS_LINUX
#include <sys/inotify.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

typedef struct _SflFsWatchEntry {
    const char *file_path;
    int watch_fd;
} SflFsWatchEntry;

#define SFL_FS_WATCH_BUFFER_SIZE (sizeof(struct inotify_event) + NAME_MAX + 1)

void sfl_fs_watch_init(SflFsWatchContext *ctx, ProcSflFsWatchNotify *notify, void *usr) {
    ctx->notify_fd = inotify_init1(IN_NONBLOCK);
    ctx->notify_proc = notify;
    ctx->usr = usr;
    ctx->buffer = malloc(SFL_FS_WATCH_BUFFER_SIZE);
}

int sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path) {
    int fd = inotify_add_watch(ctx->notify_fd, file_path, IN_CREATE | IN_DELETE);

    if (fd == -1) {
        return 0;
    }

    SflFsWatchEntry entry;
    entry.file_path = file_path;
    entry.watch_fd = fd;
    sfl_fs_watch_sb_push(ctx->entries, entry);

    return 1;
}

int sfl_fs_watch_poll(SflFsWatchContext *ctx) {
    unsigned char *buffer = (unsigned char*)ctx->buffer;
    int length;
    int offset = 0;
    int done = 0;
    int count = 0;
    while (!done) {
        length = read(ctx->notify_fd, buffer, SFL_FS_WATCH_BUFFER_SIZE);

        if (length == 0) {
            done = 1;
        } else if (length < 0) {
            if (errno == EAGAIN) {
                return count > 0 ? SFL_FS_WATCH_RESULT_NONE : SFL_FS_WATCH_RESULT_TIMEOUT;
            } else {
                return SFL_FS_WATCH_RESULT_ERROR;
            }
        }

        struct inotify_event *event = (struct inotify_event*)buffer + offset;
        offset += length;
        count++;
    }

    return SFL_FS_WATCH_RESULT_NONE;
}

int sfl_fs_watch_wait(SflFsWatchContext *ctx) {
    return 0;
}


#endif


#endif

