#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <glib.h>

extern int log_fd;

void run_loop();
void quit_loop();

gboolean filter_input(int in_fd, GIOCondition cond, gpointer user_data);

gpointer get_dbus_proxy();

#endif
