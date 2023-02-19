/* SFL watchdir v0.1

This example uses sfl_fs_watch.h to watch one or more directories/files for 
changes.

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

USAGE
watchdir <path>+ (-once)?

- path:  Relative or absolute path to file or directory
- -once: Watch the remaining paths only once, and then stop

Example: watchdir ./myfile.txt -once ./mydirectory
This will watch "myfile.txt" for all changes, while only one change will be 
recorded for "mydirectory"

CONTRIBUTION
Michael Dodis (michaeldodisgr@gmail.com)
*/
#define SFL_FS_WATCH_IMPLEMENTATION
#include "sfl_fs_watch.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef enum {
    WATCH_FLAG_ONCE    = 1 << 0,
    WATCH_FLAG_ACTIVE  = 1 << 1,
} WatchFlags;

typedef struct {
    int id;
    WatchFlags flags;
} Watch;

typedef struct {
    Watch *buffer;
    int count;
} Watches;

Watches create_watches(
    SflFsWatchContext *context, 
    int argc, 
    char const *argv[]) 
{
    int count = 0;
    for (int i = 1; i < argc; ++i) {
        const char *str = argv[i];
        if (*str == '-') {
            continue;
        }
        count++;
    }

    if (count <= 0) {
        Watches result;
        result.buffer = 0;
        result.count = 0;
        return result;
    }

    Watches result;
    result.buffer = malloc(sizeof(Watch) * count);
    result.count = count;

    count = 0;
    int once = 0;
    for (int i = 1; i < argc; ++i) {
        const char *str = argv[i];
        if (strcmp(str, "-once") == 0) {
            once = 1;
            continue;
        }

        int id = sfl_fs_watch_add(context, argv[i]);
        WatchFlags flags = 0;
        assert(id >= 0);

        flags |= WATCH_FLAG_ACTIVE;
        if (once) {
            flags |= WATCH_FLAG_ONCE;
        }

        result.buffer[count].id = id;
        result.buffer[count].flags = flags;

        count++;
    }

    return result;
}

static Watch *find_watch(Watches *watches, int id) {
    for (int i = 0; i < watches->count; ++i) {
        if (watches->buffer[i].id == id) {
            return &watches->buffer[i];
        }
    }

    return 0;
}

PROC_SFL_FS_WATCH_NOTIFY(my_notify) {
    const char *kind = sfl_fs_watch_notification_kind_to_string(notification->kind);
    Watches *watches = (Watches*)usr;
    Watch *watch = find_watch(watches, notification->id);

    if (!watch) {
        fprintf(stderr,
            "Error: Watch with id %d and path %s does not exist.\n",
            notification->id,
            notification->path);
        return;
    }

    if (!(watch->flags & WATCH_FLAG_ACTIVE)) {
        if (watch->flags & WATCH_FLAG_ONCE) {
            fprintf(stderr,
                "Error: Watch with id %d and path %s should not be active because it was flagged as ONCE.\n",
                notification->id,
                notification->path);
        }
        return;
    }

    printf("%s\t%s\n", kind, notification->path);

    if (watch->flags & WATCH_FLAG_ONCE) {
        watch->flags &= ~(WATCH_FLAG_ACTIVE);
        sfl_fs_watch_rm_id(ctx, notification->id);
    }

    return;
}

int main(int argc, char const *argv[]) {
    SflFsWatchContext context;
    sfl_fs_watch_init(&context, my_notify, 0);
    
    if (argc < 2) {
        puts("No files or directories to watch. Exiting...");
        return -1;
    }

    Watches watches = create_watches(&context, argc, argv);
    if (watches.count == 0) {
        puts("No directories to watch. Exiting...");
        return -1;
    }

    for (int i = 0; i < watches.count; ++i) {
        Watch *watch = &watches.buffer[i];
        printf("Watch id %d created\n", watch->id);
    }

    sfl_fs_watch_set_usr(&context, &watches);

    while (1) {
        int err = sfl_fs_watch_wait(&context);
        if (err == 0) {
            continue;
        } else if (err > 0) {
            if (err == SFL_FS_WATCH_RESULT_NO_MORE_DIRECTORIES_TO_WATCH) {
                break;
            } else {
                continue;
            }
        } else {
            printf("e: %d\n", err);
        }
        
    }
    return 0;
}
