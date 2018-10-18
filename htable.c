#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "htable.h"

#define LOAD_FACTOR 72


unsigned long int fnv1(char *s) {
    unsigned long int hash = 0xcbf29ce484222325u;
    for (; *s != '\0'; s++) {
        hash *= 0x100000001b3u;
        hash ^= *s;
    }
    return hash;
}

typedef struct HTEntry *HTEntry;
struct HTEntry {
    char *key;
    Node value;
    int dist;
};

struct HTable {
    HTEntry table;
    int cap;
    int size;
};

void HTable_resize(HTable h, int newsize);

HTable HTable_new(int cap) {
    assert(cap > 0);
    HTable h = (HTable) malloc(sizeof(struct HTable));
    h->cap = cap;
    h->size = 0;
    h->table = (HTEntry) calloc(cap, sizeof(struct HTEntry));
    return h;
}

void HTable_debug(HTable h, FILE *fp) {
    for (int i = 0; i < h->cap; i++) {
        HTEntry e = &(h->table[i]);
        if (e->key == NULL)
            fprintf(fp, "%d.[%d]\n", i, e->dist);
        else {
            fprintf(fp, "%d.[%d]  '%s' => %p\n", i, e->dist, e->key, (void*) e->value);
        }
    }
}

int HTable_set_(HTable h, int owned, char *key, Node value) {
    assert(h->size < h->cap);
    h->size += 1;

    unsigned int i = fnv1(key) % h->cap;
    int dist = 0;
    struct HTEntry rich;

    for (;;) {
        if (h->table[i].key == NULL) {
            if (!owned)
                key = strdup(key);
            h->table[i].key = key;
            h->table[i].value = value;
            h->table[i].dist = dist;
            return 1;
        }

        if (strcmp(key, h->table[i].key) == 0) {
            Node_forget(h->table[i].value);
            h->table[i].value = value;
            return 0;
        }

        if (h->table[i].dist < dist) {
            // swap out rich for new value
            rich = h->table[i];

            if (!owned)
                key = strdup(key);
            h->table[i].key = key;
            h->table[i].value = value;
            h->table[i].dist = dist;
            break;
        }

        i = (i + 1) % h->cap;
        dist += 1;
    }

    for (;;) {
        if (h->table[i].key == NULL) {
            h->table[i] = rich;
            return 1;
        }

        if (h->table[i].dist < rich.dist) {
            struct HTEntry tmp = rich;
            rich = h->table[i];
            h->table[i] = tmp;
        }

        i = (i + 1) % h->cap;
        rich.dist += 1;
    }
}

int HTable_set(HTable h, char *key, Node value) {
    if (h->size * 100 / h->cap >= LOAD_FACTOR)
        HTable_resize(h, h->cap * 2);

    return HTable_set_(h, 0, key, value);
}

Node HTable_get(HTable h, char *key) {
    unsigned int i = fnv1(key) % h->cap;

    for (int attempt = 0; attempt < h->cap; attempt++) {
        if (h->table[i].key == NULL)
            return NULL;
        if (strcmp(key, h->table[i].key) == 0)
            // TODO increase rc for returned value?
            return h->table[i].value;
        i = (i + 1) % h->cap;
    }

    return NULL;
}

void HTable_resize(HTable h, int newsize) {
    assert(newsize >= h->cap);

    HTEntry old_table = h->table;
    int old_size = h->cap;

    h->table = (HTEntry) calloc(newsize, sizeof(struct HTEntry));
    h->cap = newsize;
    h->size = 0;

    for (int i = 0; i < old_size; i++)
        if (old_table[i].key != NULL)
            HTable_set_(h, 1, old_table[i].key, old_table[i].value);

    free(old_table);
}

// int main() {
//     HTable h = HTable_new(6);
//     int a[] = {1, 2, 3, 4, 5, 7, 8, 9, 10, 11};
//     HTable_set(h, "123h", (Node) (a + 0));
//     HTable_set(h, "223", (Node) (a + 1));
//     HTable_set(h, "333", (Node) (a + 2));
//     HTable_set(h, "4770", (Node) (a + 3));
//     HTable_set(h, "4771", (Node) (a + 4));
//     HTable_set(h, "5134", (Node) (a + 5));
//     HTable_set(h, "143h", (Node) (a + 6));
//     HTable_set(h, "393", (Node) (a + 7));
//     HTable_set(h, "393..", (Node) (a + 8));
//     HTable_set(h, "393...", (Node) (a + 9));
//
//     char *key = "333";
//     int *x = (int*) HTable_get(h, key);
//     if (x == NULL)
//         printf("'%s' not found\n", key);
//     else
//         printf("'%s' => %d\n", key, *x);
//
//     HTable_debug(h, stdout);
// }
