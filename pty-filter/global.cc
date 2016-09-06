#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "global.h"
#include "context.h"
#include "child.h"


int log_fd = open("/dev/shm/vte_slave.log", O_CREAT | O_RDWR | O_NONBLOCK | O_TRUNC, 0644);

GMainLoop* loop = g_main_loop_new(0, true);


void run_loop()
{
    g_main_loop_run(loop);
}

void quit_loop()
{
    g_main_loop_quit(loop);
}


extern "C" {
    void dbus_init(gpointer, const char*);
    gpointer pty_filter_proxy_new();
};

gpointer get_dbus_proxy()
{
    static volatile gpointer proxy = 0;
    if (g_once_init_enter(&proxy)) {
        char* addr = g_strdup_printf("com.deepin.terminal.ptyfilter-%d", getpid());
        proxy = pty_filter_proxy_new();
        dbus_init(proxy, addr);
        g_free(addr);
    }
    return proxy;
}
