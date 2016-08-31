#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

void feed(char* buf, int len);

struct termios termp_vte;
struct winsize slave_sz;

int slave_fd = -1;
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
    /*   raw.c_oflag |= OPOST; */

    tcsetattr(fd, TCSAFLUSH, &raw);
}


int init_fd()
{
    int master_fd = open("/dev/ptmx", O_RDWR | O_NONBLOCK);

    if (master_fd == -1)
        printf("E: INIT_FD\n");

    if (!isatty(0)) {
        printf("E: vte_slave_fd\n");
        exit(-2);
    }

    grantpt(master_fd);
    unlockpt(master_fd);

    tcgetattr(0, &termp_vte);

    make_raw_slave(0);

    log_fd = open("/dev/shm/vte_slave.log", O_CREAT | O_RDWR | O_NONBLOCK | O_TRUNC, 0644);

    return master_fd;
}


gboolean read_and_write(int in_fd, GIOCondition cond, gpointer _out_fd)
{
    gboolean eof = cond & G_IO_HUP;

	if (cond & (G_IO_IN ) == 0) {
        printf("HEHE>>>>>\n");
        return FALSE;
    }

    int out_fd = GPOINTER_TO_INT(_out_fd);

    static char buf[1024];
    int r = 0;
    r = read(in_fd, buf, sizeof(buf));
    if (r == -1) {
        printf("COND:%d=%d SYNC: %d-->%d %d\n", cond,G_IO_IN, in_fd, out_fd, r);
    } else if (r > 0) {
        feed(buf, r);
        write(out_fd, buf, r);
    }
    return TRUE;
}

void sync_output(int vte_fd, int master_fd)
{
    g_unix_fd_add(vte_fd, G_IO_IN,
                  (GUnixFDSourceFunc)read_and_write, GINT_TO_POINTER(master_fd));

    g_unix_fd_add(master_fd, G_IO_IN,
                  (GUnixFDSourceFunc)read_and_write, GINT_TO_POINTER(vte_fd));
}

static void set_pty_fd(void* pts_path)
{
    int fd = open((char*)pts_path, O_RDWR | O_NONBLOCK);

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
    if (slave_fd == -1) {
        return;
    }
    int fd = slave_fd;
    struct winsize sz;
    ioctl(0, TIOCGWINSZ, &sz);
    ioctl(fd, TIOCSWINSZ, &sz);
    kill(tcgetsid(fd), SIGWINCH);
    return;
}

void run_child(const char* ptsname)
{
    slave_fd = open(ptsname, O_RDWR | O_NOCTTY);

    char* argv[] = {"/bin/bash", "-l", 0};
    GError* err = NULL;
    GPid pid;
    if (!g_spawn_async(
        0, argv, 0,
        G_SPAWN_DO_NOT_REAP_CHILD, set_pty_fd, (void*)ptsname,
        &pid, &err)) {
        dprintf(log_fd, "E: run_child %s\n", err->message);
        g_error_free(err);
        exit(-3);
    }
    g_child_watch_add(pid, when_child_exit, 0);

    dprintf(log_fd, "PP:run child %d at %s \n", pid, ptsname);
}


void init_signal()
{
    signal(SIGWINCH, sync_win_sz);
//    signal(SIGINT, SIG_IGN);
}

int main(int argc, char* argv[])
{
    int master_fd = init_fd();
    dup2(0, 77);
    int vte_fd = 77;

    init_signal();

    sync_output(vte_fd, master_fd);

    run_child(ptsname(master_fd));

    loop = g_main_loop_new(0, TRUE);
    g_main_loop_run(loop);
}
