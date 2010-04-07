#include "resource.h"
#include "list.h"

struct resource * statictext_resource (char * path, char * text);
list_t * statictext_resources (Dictionary * config);
