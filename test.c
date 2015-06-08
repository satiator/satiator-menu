#include <iapetus.h>
#include "satisfier.h"

void test_file_io(void) {
    // hacky file testing.
    char buf[512];
    int i;
    buf[0] = 'f';
    buf[1] = 0;

    int fd = s_open("foo.bar", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    for (i=0; i<128; i++)
        buf[i] = 3*i;
    s_write(fd, buf, 128);
    s_close(fd);

    fd = s_open("foo.bar", FA_READ|FA_WRITE);
    s_seek(fd, 96);
    s_truncate(fd);
    s_seek(fd, 48);
    s_read(fd, buf, 32);
    for (i=0; i<32; i++)
        buf[i] &= 0xf0;
    s_seek(fd, 48);
    s_write(fd, buf, 32);
    s_close(fd);

    s_rename("foo.bar", "baz.foo");
    
    i = s_stat("baz.foo", (void*)buf, sizeof(buf));
    s_stat_t * stat = (void*)buf;
    fd = s_open("out", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    s_write(fd, stat, i+9);
    s_close(fd);

    s_mkdir("blonk");

    s_opendir(".");
    fd = s_open("out2", FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
    while ((i = s_stat(NULL, stat, sizeof(buf))) > 0) {
        s_write(fd, stat, i+9);
    }
    s_close(fd);

    s_chdir("blonk");
    s_rename("../out2", "bloop");

    s_chdir("..");
    s_unlink("baz.foo");
}
