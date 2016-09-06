#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <cassert>

#include "mypty.h"
#include "child.h"
#include "global.h"
#include "manager.h"


Pty* vte_pty = 0;
Pty* master_pty = 0;
Pty* slave_pty = 0;

void init_pty()
{
    static bool init = false;
    if (!init) {
        init = true;
        vte_pty = Pty::fromFd(0);
        master_pty = Pty::create();
        slave_pty = master_pty->slave();
    }
}


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
    init_pty();

    assert(vte_pty);
    assert(master_pty);
    assert(slave_pty);

    atexit(reset_vte_pty);

    init_signal();

    // create the dbus proxy;
    get_dbus_proxy();

    vte_pty->save();
    master_pty->copyWinSize(vte_pty);
    vte_pty->setRaw();

    init_filter_manager(vte_pty->fd(), master_pty->fd());

    const char* shell_argv[] = {"/bin/bash", "-l", 0};
    launch_session_leader(slave_pty->fd(), "/dev/shm/", (char**)shell_argv, 0);
    
    run_loop();
}
