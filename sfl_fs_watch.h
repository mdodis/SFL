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

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)
*/

#ifndef SFL_FS_WATCH_H
#define SFL_FS_WATCH_H

#ifdef _WIN32
    #define SFL_FS_WATCH_OS_WINDOWS 1
#endif

#ifndef SFL_FS_WATCH_OS_WINDOWS
    #define SFL_FS_WATCH_OS_WINDOWS 0
#endif

typedef struct {
    void *handle;
    const char *directory;
    void *extra;
    unsigned char *buffer;
} SflFsWatchEntry;

typedef struct {
    SflFsWatchEntry *entries;
    const char **watched_files;
    void *internal;
} SflFsWatchContext;

extern void sfl_fs_watch_init(SflFsWatchContext *ctx);
extern void sfl_fs_watch_deinit(SflFsWatchContext *ctx);
extern void sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path);
extern int  sfl_fs_watch_poll(SflFsWatchContext *ctx);
extern void sfl_fs_watch_wait(SflFsWatchContext *ctx);

#endif

// #if SFL_FS_WATCH_IMPLEMENTATION
#if 1
/* @todo: Allow for custom malloc/free implementations */

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

#define SFL_FS_WATCH_BUFFER_SIZE (64 * 1024 * 1024)

typedef struct {
    HANDLE iocp;
} SflFsWatchContextInternal;

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
    HANDLE dir_handle = (HANDLE)entry->handle;
    if (entry->handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

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
        (OVERLAPPED*)entry->extra,
        0);
    return 0;
}

void sfl_fs_watch_init(SflFsWatchContext *ctx) {
    ctx->entries = 0;
    ctx->watched_files = 0;
    SflFsWatchContextInternal *internal = (SflFsWatchContextInternal*)malloc(sizeof(SflFsWatchContextInternal));
    internal->iocp = 0;
    ctx->internal = internal;
}

/* @todo: for now this only accepts directories */
void sfl_fs_watch_add(SflFsWatchContext *ctx, const char *file_path) {
    SflFsWatchContextInternal *internal = (SflFsWatchContextInternal*)ctx->internal;

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
        entry.extra = (void*)ovl;
        entry.buffer = (unsigned char*)malloc(SFL_FS_WATCH_BUFFER_SIZE);
        
        sfl_fs_watch_sb_push(ctx->entries, entry);
    }

    SflFsWatchEntry *entry = &sfl_fs_watch_sb_last(ctx->entries);
    sfl_fs_watch_attach_to_iocp(entry->handle, &internal->iocp, (ULONG_PTR)entry, 0);

    sfl_fs_watch_issue_entry(entry);
}


int sfl_fs_watch_poll(SflFsWatchContext *ctx) {
    SflFsWatchContextInternal *internal = (SflFsWatchContextInternal*)ctx->internal;
POLL_AGAIN:
    DWORD bytes_transferred;
    ULONG_PTR key;
    OVERLAPPED *ovl;
    HANDLE iocp = internal->iocp;
    DWORD result = sfl_fs_watch_poll_iocp(
        internal->iocp, 
        0,
        &bytes_transferred,
        &key,
        &ovl);

    if (result == ERROR_OPERATION_ABORTED) {
        goto POLL_AGAIN;
    }

    if (result != ERROR_SUCCESS) {
        return result;
    }


    return 0;
}


void sfl_fs_watch_wait(SflFsWatchContext *ctx) {

}

void sfl_fs_watch_deinit(SflFsWatchContext *ctx) {

}


#endif


#endif

