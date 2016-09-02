#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <vector>
#include <glib.h>
#include "mypty.h"

extern Pty* vte_pty;
extern Pty* master_pty;
extern Pty* slave_pty;

extern int log_fd;

extern GMainLoop* loop;

extern struct FilterContext* UserInputFilters;

extern struct FilterContext* AppOutputFilters;


gboolean filter_input(int in_fd, GIOCondition cond, gpointer user_data);


#endif
