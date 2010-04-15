#ifndef __SHRECORD_H__
#define __SHRECORD_H__

list_t * shrecord_resources (Dictionary * config);

int shrecord_init (void);
int shrecord_run (void);
void shrecord_cleanup (void);

#endif /* __SHRECORD_H__ */
