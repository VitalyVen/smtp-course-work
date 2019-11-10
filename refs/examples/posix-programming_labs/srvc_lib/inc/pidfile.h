#ifndef __PIDFILE_H__
#define __PIDFILE_H__

int open_pidfile(const char *pidfile);

int close_pidfile();

int delete_pidfile(const char *pidfile);

int check_status(const char *pidfile);

#endif
