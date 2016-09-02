#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>
#include <cassert>

#include "context.h"
#include "mypty.h"
#include "child.h"
#include "global.h"

inline void reset_vte_pty()
{
    vte_pty->restore();
}
inline void sync_win_sz(int _no_used)
{
    master_pty->copyWinSize(vte_pty);
}

void init_signal()
{
    signal(SIGWINCH, sync_win_sz);
    signal(SIGINT, SIG_IGN);
}

int main(int argc, char* argv[])
{
    assert(vte_pty);
    assert(master_pty);
    assert(slave_pty);

    atexit(reset_vte_pty);

    init_signal();
    
    vte_pty->save();
    master_pty->copyWinSize(vte_pty);
    vte_pty->setRaw();

    
    g_unix_fd_add(vte_pty->fd(), G_IO_IN, (GUnixFDSourceFunc)filter_input, UserInputFilters);
    g_unix_fd_add(master_pty->fd(), G_IO_IN, (GUnixFDSourceFunc)filter_input, AppOutputFilters);


    const char* shell_argv[] = {"/bin/bash", "-l", 0};
    launch_session_leader(slave_pty->fd(), "/dev/shm/", (char**)shell_argv, 0);

    g_main_loop_run(loop);
}
