#ifndef __MY_PTY_H__
#define __MY_PTY_H__

#include <termios.h>

class Pty {
public:
    void save();
    void restore();
    void setRaw();

    int fd();
    void write(const char* data, size_t s);

    void fixme_cancel();
    
    static Pty* fromFd(int fd);
    static Pty* fromPtsName(const char* name);
    static Pty* create();

    Pty* slave();

    void becomeSessionLeader();
    
    void copyWinSize(const Pty* p);
    
private:
    Pty() {};
    struct termios m_config;
    bool m_saved;
    int m_fd;
};

#endif
