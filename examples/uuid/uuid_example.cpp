#include <stdio.h>
#define SFL_UUID_IMPLEMENTATION
#include "sfl_uuid.h"

int main(int argc, char* argv[])
{
    SflUUIDContext context;
    sfl_uuid_init(&context);

    for (int i = 0; i < 100; ++i) {
        SflUUID uuid;
        sfl_uuid_gen_v4(&context, &uuid);

        static char buf[SFL_UUID_BUFFER_SIZE];

        sfl_uuid_to_string(&uuid, buf);

        printf("%d: %s\n", i, buf);
    }
    return 0;
}