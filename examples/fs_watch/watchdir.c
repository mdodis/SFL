#define SFL_FS_WATCH_IMPLEMENTATION
#include "sfl_fs_watch.h"
#include <stdio.h>

int main(int argc, char const *argv[]) {
    SflFsWatchContext context;
    sfl_fs_watch_init(&context);
    sfl_fs_watch_add(&context, "./");

    while (1) {
        int er = sfl_fs_watch_poll(&context);
        if (er == 0) {
            puts("Something changed");
        } else if (er == WAIT_TIMEOUT) {
            continue;
        } else {
            printf("e: %d\n", er);
        }
        
    }
    return 0;
}
