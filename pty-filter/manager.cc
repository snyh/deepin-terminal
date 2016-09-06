#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>

#include "context.h"
#include "global.h"
#include "manager.h"
#include "child.h"
#include "mypty.h"

//TODO: REMOVE
extern Pty* vte_pty;
extern Pty* master_pty;
extern Pty* slave_pty;


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


void init_filter_manager(int in_fd, int out_fd)
{
    struct FilterContext* UserInputFilters = initUserInputFilters();
    struct FilterContext* AppOutputFilters =  initAppOutputFilters();
    g_unix_fd_add(out_fd, G_IO_IN, (GUnixFDSourceFunc)filter_input, AppOutputFilters);
    g_unix_fd_add(in_fd, G_IO_IN, (GUnixFDSourceFunc)filter_input, UserInputFilters);
}
