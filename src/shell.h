#ifndef __SHELL_H__
#define __SHELL_H__

int shell_stream (int fd, char * cmd);

int shell_copy (char * buf, size_t n, char * cmd);

#endif /* __SHELL_H__ */
