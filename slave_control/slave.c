#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

void sync_output(int vte_fd, int master_fd);

static struct termios termp_vte;

extern gboolean IN_RZ_MODE;

int master_fd = -1;
int log_fd = -1;

void make_raw_slave(int fd)
{
    struct termios raw;
    raw.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP
                      |INLCR|IGNCR|ICRNL|IXON);
    raw.c_oflag &= ~OPOST;
    raw.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    raw.c_cflag &= ~(CSIZE|PARENB);
    raw.c_cflag |= CS8;

    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSAFLUSH, &raw);
}


int init_fd()
{
    master_fd = open("/dev/ptmx", O_RDWR);

    if (master_fd == -1) {
        printf("E: INIT_FD\n");
        exit(-2);
    }

    if (!isatty(0)) {
        printf("E: vte fd\n");
        exit(-2);
    }

    grantpt(master_fd);
    unlockpt(master_fd);

    tcgetattr(0, &termp_vte);

    make_raw_slave(0);

    log_fd = open("/dev/shm/vte_slave.log", O_CREAT | O_RDWR | O_NONBLOCK | O_TRUNC, 0644);

    return master_fd;
}

void set_rz_fd()
{
    signal(SIGINT,SIG_DFL);
    signal(SIGWINCH,SIG_DFL);

    dup2(master_fd, 0);
    dup2(master_fd, 1);
    dup2(log_fd, 2);
}



void enter_rz_mode()
{
    IN_RZ_MODE = TRUE;
    printf("IN RZ MODE！！！\n");

    struct termios saved_term;
    tcgetattr(master_fd, &saved_term);

    make_raw_slave(master_fd);

    char* argv[] = {"/usr/bin/rz", "-b", 0};
    GError* err = NULL;
    gint status;
    if (!g_spawn_sync(
        0, argv, 0,
        0, set_rz_fd, 0,
        0, 0,
        &status, &err)) {
        printf("E: enter_rz_mode %s\n", err->message);
        g_error_free(err);
        exit(-3);
    }
    if (!g_spawn_check_exit_status(status, &err)) {
        char c = 24;
        tcflush(master_fd, TCIOFLUSH);
        for (int i = 0;i < 99;i++)
        {
            write(master_fd, &c, 1);
            tcdrain(master_fd);
        }
        printf("Fatal RZ MODE！！！%d->%s\n", status, err->message);
        g_error_free(err);
    }
    tcsetattr(master_fd, TCSANOW, &termp_vte);

    const char msg[] = "\r\n";
    write(master_fd, msg, sizeof(msg));

    IN_RZ_MODE = FALSE;
}

static void set_shell_fd(void* pts_path)
{
    int fd = open((char*)pts_path, O_RDWR);

    setsid();

    if (ioctl(fd, TIOCSCTTY, 0) < 0) {
        printf("E: TIOCSCTTY\n");
    }
    dup2(fd, 0);

    dup2(fd, 1);

    dup2(fd, 2);

    close(fd);

    dprintf(log_fd,"TRY OPEN: %s-->%d\n", pts_path, fd);
}

static GMainLoop* loop = NULL;

void when_child_exit(GPid pid, gint status, gpointer no_used)
{
    g_spawn_close_pid(pid);
    g_main_loop_quit(loop);
    exit(status);
}


void sync_win_sz()
{
    struct winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);
    ioctl(master_fd, TIOCSWINSZ, &sz);

    kill(tcgetsid(master_fd), SIGWINCH);
    return;
}

void run_shell(const char* ptsname)
{
    char* argv[] = {"/bin/bash", "-l", 0};
//    char* argv[] = {"/usr/bin/sz", "-b", "wtmp", 0};
    GError* err = NULL;
    GPid pid;
    if (!g_spawn_async(
        "/dev/shm/", argv, 0,
        G_SPAWN_DO_NOT_REAP_CHILD, set_shell_fd, (void*)ptsname,
        &pid, &err)) {
        dprintf(log_fd, "E: run_shell %s\n", err->message);
        g_error_free(err);
        exit(-3);
    }
    g_child_watch_add(pid, when_child_exit, 0);

    dprintf(log_fd, "PP:run child %d at %s \n", pid, ptsname);


}

void reset_term()
{
    tcsetattr(0, TCSANOW, &termp_vte);
}

void init_signal()
{
    signal(SIGWINCH, sync_win_sz);
    signal(SIGINT, SIG_IGN);

    atexit(reset_term);
}


int main(int argc, char* argv[])
{
    init_fd();
    atexit(reset_term);
    init_signal();
    sync_win_sz();

    sync_output(0, master_fd);

    run_shell(ptsname(master_fd));

    loop = g_main_loop_new(0, TRUE);
    g_main_loop_run(loop);
}
