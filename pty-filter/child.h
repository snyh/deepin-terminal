#ifndef __CHILD_H__
#define __CHILD_H__

class Pty;
class StreamContext;

void launch_z(Pty* pty, StreamContext* ctx, const char* workdir, char* argv[], char* envp[]);
void launch_session_leader(int pty_fd, const char* workdir, char* argv[], char* envp[]);

#endif
