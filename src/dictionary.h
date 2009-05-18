
#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

typedef void Dictionary;

Dictionary * dictionary_new (void);
int dictionary_insert (Dictionary * table, const char * name,
		       const char * value);
const char * dictionary_lookup (Dictionary * table, const char * name);
int dictionary_delete (Dictionary * table);

#endif /* __DICTIONARY_H__ */
