
#include <stdlib.h>
#include <string.h>

#include "x_tree.h"
#include "jhash.h"

#define x_strlen(s) ((s)?strlen((s)):(0))
#define x_strdup(s) ((s)?strdup((s)):(NULL))

#define BUCKETS_MASK 4
#define BUCKETS (1<<BUCKETS_MASK)

typedef struct _Variable Variable;
typedef struct _Dictionary Dictionary;

struct _Variable {
  char * name;
  char * value;
};

struct _Dictionary {
  x_tree_t * buckets[BUCKETS];
};

static Variable *
variable_new (const char * name, const char * value)
{
  Variable * variable;

  variable = (Variable *) malloc (sizeof (Variable));
  variable->name = x_strdup (name);
  variable->value = x_strdup (value);

  return variable;
}

static int
variable_free (Variable * variable)
{
  if (!variable) return -1;

  free (variable->name);
  free (variable->value);
  free (variable);

  return 0;
}

static int
variable_cmp (char * s1, Variable * v2)
{
  if (!s1 || !v2) return -1;
  return strcmp (s1, v2->name);
}

Dictionary *
dictionary_new (void)
{
  Dictionary * table;
  int i;

  table = (Dictionary *) malloc (sizeof (Dictionary));
  for (i = 0; i < BUCKETS; i++) {
    table->buckets[i] = x_tree_new ((x_cmp_t)variable_cmp);
  }

  return table;
}

int
dictionary_delete (Dictionary * table)
{
  int i;

  if (!table) return -1;
  for (i = 0; i < BUCKETS; i++) {
    x_tree_free_with (table->buckets[i], (x_free_t)variable_free);
  }
  free (table);

  return 0;
}

const char *
dictionary_lookup (Dictionary * table, const char * name)
{
  Variable * variable;
  x_node_t * node;
  ub4 h;

  h = jenkins_hash ((ub1 *)name, strlen (name), 0);
  h = (h & hashmask (BUCKETS_MASK));

  node = x_tree_find (table->buckets[h], (void *)name);
  if (node == NULL) {
    return NULL;
  } else {
    variable = x_node_data (table->buckets[h], node);
    return (const char *) variable->value;
  }
}

int
dictionary_insert (Dictionary * table, const char * name, const char * value)
{
  Variable * variable;
  x_node_t * node;
  ub4 h;

  h = jenkins_hash ((ub1 *)name, strlen (name), 0);
  h = (h & hashmask (BUCKETS_MASK));

  node = x_tree_find (table->buckets[h], (void *)name);
  if (node == NULL) {
    variable = variable_new (name, value);
    table->buckets[h] = x_tree_insert (table->buckets[h], variable);
  } else {
    variable = x_node_data (table->buckets[h], node);
    free (variable->value);
    variable->value = x_strdup (value);
  }

  return 0;
}
