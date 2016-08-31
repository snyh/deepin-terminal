
#include <glib.h>


enum {
    MaybeYes = 0,
    MaybeNo = -1,
    Maybe = 1
};
extern int log_fd;
int maybe(char d)
{
    static char sz_send[] = {0162, 0172, 015, 052, 052, 030, 0102, 060, 060, 060, 060, 060, 060, 060, 060, 060};
    static gboolean next_pos = 0;



    char wish_char = sz_send[next_pos];

    if (d == wish_char) {
        next_pos++;
        dprintf(log_fd, "DEBUG: %o == %o Next_POS++=%d(%o)\n", d, wish_char, next_pos, sz_send[next_pos]);
    } else {
        dprintf(log_fd, "Didn't Match  %o != %o\n", d, wish_char);
        next_pos = 0;
        return MaybeNo;
    }

    if (next_pos > sizeof(sz_send) - 1) {
        next_pos = 0;
        return MaybeYes;
    }  else {
        return Maybe;
    }
}


void feed(char* buf, int len)
{
    for (int i=0; i<len; i++) {
        if (maybe(buf[i]) == MaybeYes) {
            printf("Found SZ TAG...!!!!\n");
        }
    }
}
