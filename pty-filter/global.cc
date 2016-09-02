#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

#include "global.h"
#include "context.h"
#include "child.h"

Pty* vte_pty = Pty::fromFd(0);
Pty* master_pty = Pty::create();
Pty* slave_pty = master_pty->slave();

int log_fd = open("/dev/shm/vte_slave.log", O_CREAT | O_RDWR | O_NONBLOCK | O_TRUNC, 0644);

GMainLoop* loop = g_main_loop_new(0, true);

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
    StreamContext* RZ = new StreamContext(new Matcher(RZ_START_FP, sizeof(RZ_START_FP)));
    RZ->connect(launch_sz);

    std::vector<StreamContext*> m;
    m.push_back(RZ);

    return new FilterContext{vte_pty->fd(), m};
}

static FilterContext* initAppOutputFilters()
{
    StreamContext* SZ = new StreamContext(new Matcher(SZ_START_FP, sizeof(SZ_START_FP)));
    SZ->connect(launch_rz);

    std::vector<StreamContext*> m;
    m.push_back(SZ);

    return new FilterContext{master_pty->fd(), m};
}

struct FilterContext* UserInputFilters = initUserInputFilters();
struct FilterContext* AppOutputFilters =  initAppOutputFilters();
