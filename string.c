#include <stdlib.h>
#include <string.h>
#include "string.h"


String String_init(String self, char *s, int len) {
    self->len = len;
    self->buf = (char*) malloc((len + 1) * sizeof(char));
    self->buf[len] = '\0';
    strncpy(self->buf, s, len);
    return self;
}

String String_from(char *s, int len) {
    String new = (String) malloc(sizeof(struct String));
    return String_init(new, s, len);
}

int String_equals(String x, String y) {
    if (x == y)
        return 1;
    if (x == NULL || y == NULL)
        return 0;
    if (x->len != y->len)
        return 0;
    return strncmp(x->buf, y->buf, x->len) == 0;
}
