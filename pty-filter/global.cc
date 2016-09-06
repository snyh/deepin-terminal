#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

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
