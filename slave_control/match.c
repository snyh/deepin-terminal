
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <glib-unix.h>

void enter_rz_mode();

gboolean IN_RZ_MODE = FALSE;

enum {
    Yes = 0,
    No = -1,
    Maybe = 1
};

char SZ_FINGERPRINTER[] = {0162, 0172, 015, 052, 052, 030, 0102, 060, 060, 060, 060, 060, 060, 060, 060, 060};
extern int log_fd;
int maybe(char d)
{
    static gboolean next_pos = 0;

    char wish_char = SZ_FINGERPRINTER[next_pos];

    if (d == wish_char) {
        next_pos++;
//        dprintf(log_fd, "DEBUG: %o == %o Next_POS++=%d(%o)\n", d, wish_char, next_pos, SZ_FINGERPRINTER[next_pos]);
    } else {
//        dprintf(log_fd, "Didn't Match  %o != %o\n", d, wish_char);
        next_pos = 0;
        return No;
    }

    if (next_pos > sizeof(SZ_FINGERPRINTER) - 1) {
        next_pos = 0;
        return Yes;
    }  else {
        return Maybe;
    }
}


gboolean feed_and_write(char* buf, int len, int fd)
{
    int status = -1;
    static char cached[sizeof(SZ_FINGERPRINTER)];
    static int pos = 0;

    for (int i=0; i<len; i++) {
        status = maybe(buf[i]);
        switch (status) {
        case Yes:
        {
            pos = 0;
            return TRUE;
        }
        case No:
        {
            if (pos > 0) {
                write(fd, cached, pos);
                pos = 0;
            }
            write(fd, buf+i, 1);
            break;
        }
        case Maybe:
        {
            if (pos >= sizeof(cached)) {
                write(fd, cached, sizeof(cached));
                pos = 0;
            }
            cached[pos++] = buf[i];
            buf[i] = ' ';
            break;
        }
        }
    }
    return FALSE;
}



gboolean read_and_write(int in_fd, GIOCondition cond, gpointer _out_fd)
{
    gboolean eof = cond & G_IO_HUP;

	if (cond & (G_IO_IN ) == 0) {
        printf("HEHE>>>>>\n");
        return FALSE;
    }

    if (IN_RZ_MODE) {
        return TRUE;
    }

    int out_fd = GPOINTER_TO_INT(_out_fd);

    static char buf[1024];
    int r = 0;
    r = read(in_fd, buf, sizeof(buf));

    if (r > 0) {
        if (feed_and_write(buf, r, out_fd)) {
            enter_rz_mode();
        }
    } else if (r == -1) {
        printf("COND:%d=%d SYNC: %d-->%d %d\n", cond,G_IO_IN, in_fd, out_fd, r);
    }
    return TRUE;
}

void sync_output(int vte_fd, int master_fd)
{
    g_unix_fd_add(vte_fd, G_IO_IN,
                  (GUnixFDSourceFunc)read_and_write, GINT_TO_POINTER(master_fd));

    g_unix_fd_add(master_fd, G_IO_IN,
                  (GUnixFDSourceFunc)read_and_write, GINT_TO_POINTER(vte_fd));
}
