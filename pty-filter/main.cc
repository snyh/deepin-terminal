#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>
#include <cassert>

#include "context.h"
#include "mypty.h"
#include "child.h"
#include "global.h"


static Pty* vte_pty = 0;
static Pty* master_pty = 0;
static Pty* slave_pty = 0;

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



static void launch_rz(StreamContext* ctx)
{
    dprintf(log_fd, "LAUNCH_RZ!\n");
    static const char* argv[] = {"/usr/bin/rz", "-b", 0};
    launch_z(master_pty, ctx, 0, (char**)argv, 0);
}
static void launch_sz(StreamContext* ctx)
{
    dprintf(log_fd, "LAUNCH_SZ!\n");
    static const char* argv[] = {"/usr/bin/sz", "-b", "/etc/hosts", 0};
    launch_z(master_pty, ctx, 0, (char**)argv, 0);
}

static FilterContext* initUserInputFilters()
{
    std::vector<StreamContext*> m;
    return new FilterContext{master_pty->fd(), m};
}

static FilterContext* initAppOutputFilters()
{
    StreamContext* SZ = new StreamContext(new Matcher(SZ_START_FP, sizeof(SZ_START_FP)));
    SZ->connect(launch_rz);

    StreamContext* RZ = new StreamContext(new Matcher(RZ_START_FP, sizeof(RZ_START_FP)));
    RZ->connect(launch_sz);


    std::vector<StreamContext*> m;
    m.push_back(SZ);
    m.push_back(RZ);

    return new FilterContext{vte_pty->fd(), m};
}

int main(int argc, char* argv[])
{
    init_pty();

    assert(vte_pty);
    assert(master_pty);
    assert(slave_pty);

    atexit(reset_vte_pty);

    init_signal();

    vte_pty->save();
    master_pty->copyWinSize(vte_pty);
    vte_pty->setRaw();


    struct FilterContext* UserInputFilters = initUserInputFilters();
    struct FilterContext* AppOutputFilters =  initAppOutputFilters();
    g_unix_fd_add(master_pty->fd(), G_IO_IN, (GUnixFDSourceFunc)filter_input, AppOutputFilters);
    g_unix_fd_add(vte_pty->fd(), G_IO_IN, (GUnixFDSourceFunc)filter_input, UserInputFilters);


    const char* shell_argv[] = {"/bin/bash", "-l", 0};
    launch_session_leader(slave_pty->fd(), "/dev/shm/", (char**)shell_argv, 0);

    run_loop();
}
