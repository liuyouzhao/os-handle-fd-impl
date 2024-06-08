#ifndef SYS_H
#define SYS_H

#include "arch.h"


void sys_init();
int sys_open(tsk_id_t tid, const char* path, unsigned int mode);
int sys_close(tsk_id_t tid, int fd);
int sys_read(tsk_id_t tid, int fd, char *buf, size_t len, unsigned long* pos);
int sys_write(tsk_id_t tid, int fd, const char *buf, size_t len, unsigned long pos);

void panic();
#endif // SYS_H
