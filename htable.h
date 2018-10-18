#ifndef HTABLE_H
#define HTABLE_H

typedef struct HTable *HTable;

#include "node.h"

HTable HTable_new(int cap);
int HTable_set(HTable h, char *key, Node value);
Node HTable_get(HTable h, char *key);
#endif
