#ifndef STRING_H
#define STRING_H
typedef struct String *String;

struct String {
    char *buf;
    int len;
};

String String_init(String self, char *s, int len);
String String_from(char *s, int len);
int String_equals(String x, String y);
#endif
