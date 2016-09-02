#include <cassert>
#include <cstdio>
#include <csignal>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>


extern int log_fd;

#include "mypty.h"

static struct termios make_raw_termios()
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
    return raw;
}

Pty* Pty::fromPtsName(const char* name)
{
    Pty* pty = new Pty;

    int fd = open((char*)name, O_RDWR);
    if (fd == -1)
        return 0;
    pty->m_fd = fd;
    dprintf(log_fd,"TRY OPEN: %s-->%d\n", name, pty->m_fd);
    return pty;
}
Pty* Pty::fromFd(int fd)
{
    if (!isatty(fd)) {
        return 0;
    }
    
    Pty* pty = new Pty;
    pty->m_fd = fd;
    return pty;
}

Pty* Pty::slave()
{
    return Pty::fromPtsName(ptsname(m_fd));
}

void Pty::fixme_cancel()
{
    char c = 24;
    tcflush(m_fd, TCIOFLUSH);
    for (int i = 0; i < 99; i++)
    {
        ::write(m_fd, &c, 1);
        tcdrain(m_fd);
    }
}

void Pty::write(const char* data, size_t s)
{
    ssize_t r = ::write(m_fd, data, s);
    size_t remainder = s - r;
    if (remainder > 0) {
        // we use block IO, this shouldn't happen !
        assert(false);
        this->write(data+r, remainder);
    }
}


    
Pty* Pty::create()
{
    int fd = open("/dev/ptmx", O_RDWR);
    if (fd == -1) {
        printf("E: INIT_FD\n");
        return 0;
    }
    grantpt(fd);
    unlockpt(fd);

    return Pty::fromFd(fd);
}

int Pty::fd()
{
    assert(m_fd != -1);
    return m_fd;
}

void Pty::becomeSessionLeader()
{
    setsid();

    if (ioctl(m_fd, TIOCSCTTY, 0) < 0) {
        printf("E: TIOCSCTTY\n");
    }
    
    dup2(m_fd, 0);

    dup2(m_fd, 1);

    dup2(m_fd, 2);

    close(m_fd);

    m_fd = 0;
    

}


void Pty::setRaw()
{
    static struct termios raw_term = make_raw_termios();
    tcsetattr(m_fd, TCSAFLUSH, &raw_term);
}

void Pty::save()
{
    assert(m_saved == false);
    m_saved = true;
    tcgetattr(m_fd, &m_config);
}

void Pty::restore()
{
    assert(m_saved == true);
    m_saved = false;
    tcsetattr(m_fd, TCSANOW, &m_config);
}

void Pty::copyWinSize(const Pty* p)
{
    struct winsize sz;
    ioctl(p->m_fd, TIOCGWINSZ, &sz);
    ioctl(this->m_fd, TIOCSWINSZ, &sz);
    kill(tcgetsid(this->m_fd), SIGWINCH);
}
