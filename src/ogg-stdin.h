#ifndef __OGG_STDIN_H__
#define __OGG_STDIN_H__

#include "resource.h"
#include "list.h"

list_t * oggstdin_resources (Dictionary * config);

int oggstdin_run (void);
void oggstdin_sighandler (void);

#endif /* __OGG_STDIN_H__ */
