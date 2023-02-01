#define SFL_FS_WATCH_IMPLEMENTATION
#include "sfl_fs_watch.h"
#include <stdio.h>

PROC_SFL_FS_WATCH_NOTIFY(my_notify) {
    const char *kind = sfl_fs_watch_notification_kind_to_string(notification->kind);
    printf("%s\t%s\n", kind, notification->path);
    return;
}

int main(int argc, char const *argv[]) {
    SflFsWatchContext context;
    sfl_fs_watch_init(&context, my_notify, 0);
    if (!sfl_fs_watch_add(&context, "./")) {
        return -1;
    }

    while (1) {
        int err = sfl_fs_watch_poll(&context);
        if (err == 0) {
            continue;
        } else if (err > 0) {
            continue;
        } else {
            printf("e: %d\n", err);
        }
        
    }
    return 0;
}
