#include <glib-unix.h>
#include <stdio.h>

#include "context.h"
#include "child.h"

static void dump_char_array(const char* buf, size_t s)
{
    extern int log_fd;
    dprintf(log_fd, "☺[%d] = ", s);
    for (int i=0; i<s; i++) {
        dprintf(log_fd, " 0%o ", buf[i]);
        if (i+1 == s)
            dprintf(log_fd, "\n");
    }
}

gboolean filter_input(int in_fd, GIOCondition cond, gpointer user_data)
{
    if (cond & (G_IO_IN ) == 0 || (cond & G_IO_HUP)) {
        return false;
    }

    FilterContext* ctx = static_cast<FilterContext*>(user_data);

    char buf[1024];
    int r = read(in_fd, buf, sizeof(buf));
    if (r == -1) {

        printf("COND:%d=%d SYNC: %d\n", cond, G_IO_IN, in_fd);
        return false;
    } else if (r == 0) {

        return true;
    }

    std::vector<char> reduced(buf, buf+r);

    for (int i = 0; i < ctx->filters.size(); i++) {
        StreamContext* filter = ctx->filters[i];
        reduced = filter->Filter(buf, r);
        if (filter->QueryStatus() == Matcher::Found) {
            printf("Found!!!\n");
            reduced.clear();
            break;
        }
    }
    if (reduced.size() > 0)
        write(ctx->outFd, reduced.data(), reduced.size());

    return true;
}
