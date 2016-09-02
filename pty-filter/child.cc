#include <csignal>
#include <unistd.h>
#include <termios.h>

#include <stdio.h>
#include <sys/ioctl.h>
#include <glib-unix.h>
#include <glib.h>

#include "mypty.h"
#include "context.h"
#include "child.h"
#include "global.h"

extern int log_fd;
extern GMainLoop* loop;

namespace InChild {
    // Note: functions in here is running in child process !!!!!
    
    static void set_zmode_child(void* user_data)
    {
        int fd = GPOINTER_TO_INT(user_data);
        
        signal(SIGINT, SIG_DFL);
        signal(SIGWINCH, SIG_DFL);

        dup2(fd, 0);
        dup2(fd, 1);
        dup2(log_fd, 2);
        
        if (fd != 0 && fd != 1) {
            close(fd);
        }
    }

    static void set_session_leader(void* user_data)
    {
        int fd = GPOINTER_TO_INT(user_data);
        
        setsid();

        if (ioctl(fd, TIOCSCTTY, 0) < 0) {
            dprintf(log_fd, "E: TIOCSCTTY\n");
        }
    
        dup2(fd, 0);

        dup2(fd, 1);

        dup2(fd, 2);

        if (fd != 0 && fd != 1 && fd != 2) {
            close(fd);
        }
    }
};


static void when_session_leader_exit(GPid pid, gint status, gpointer no_used)
{
    g_spawn_close_pid(pid);
    g_main_loop_quit(loop);
}


void launch_session_leader(int pty_fd, const char* workdir, char* argv[], char* envp[])
{
    GError* err = NULL;
    GPid pid;
    if (!g_spawn_async(
            workdir, argv, envp,
            G_SPAWN_DO_NOT_REAP_CHILD, InChild::set_session_leader, GINT_TO_POINTER(pty_fd),
            &pid, &err)) {
        dprintf(log_fd, "E: run_shell %s\n", err->message);
        g_error_free(err);
        g_main_loop_quit(loop);
    }
    g_child_watch_add(pid, when_session_leader_exit, 0);
}


void launch_z(Pty* pty, StreamContext* ctx, const char* workdir, char* argv[], char* envp[])
{
    g_assert(ctx->QueryStatus() == Matcher::Found);
    printf("IN RZ MODE！！！\n");

    pty->save();
    pty->setRaw();

    GError* err = NULL;
    gint status;
    if (!g_spawn_sync(
            workdir, argv, envp,
            G_SPAWN_DEFAULT, InChild::set_zmode_child, GINT_TO_POINTER(pty->fd()),
            0, 0,
            &status, &err)) {
        pty->restore();
        printf("E: enter_rz_mode %s\n", err->message);
        g_error_free(err);
        exit(-3);
    }
    
    if (!g_spawn_check_exit_status(status, &err)) {
        pty->fixme_cancel();
        
        printf("Fatal RZ MODE！！！%d->%s\n", status, err->message);
        g_error_free(err);
    }

    pty->restore();
    
    const char msg[] = "\r\n";
    pty->write(msg, sizeof(msg));
        
    ctx->Reset();
}
