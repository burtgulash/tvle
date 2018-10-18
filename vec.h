#ifndef VEC_H
#define VEC_H
#include <stdlib.h>
#include <stdio.h>

#define VEC_OF(T) \
typedef struct Vec_##T *Vec_##T; \
struct Vec_##T { \
    typeof(T) *buf; \
    int cap; \
    int len; \
};

#define VEC_INIT(VEC, CAP) do { \
    (VEC)->buf = malloc((CAP) * sizeof(*(VEC)->buf)); \
    (VEC)->cap = (CAP); \
    (VEC)->len = 0; \
} while (0)

#define VEC_PUSH(VEC, ITEM) do { \
    if ((VEC)->len >= (VEC)->cap) \
        _VEC_RESIZE((VEC), (VEC)->cap * 2); \
    (VEC)->buf[(VEC)->len++] = (ITEM); \
} while (0)

#define _VEC_RESIZE(VEC, NEW_CAP) do { \
    typeof((VEC)->buf) _old_buf = (VEC)->buf; \
    (VEC)->cap = (NEW_CAP); \
    if ((VEC)->cap <= 0) \
        (VEC)->cap = 1; \
    (VEC)->buf = malloc((VEC)->cap * sizeof(*(VEC)->buf)); \
    for (int _i = 0; _i < (VEC)->len; _i++) \
        (VEC)->buf[_i] = _old_buf[_i]; \
    free(_old_buf); \
} while (0)

#define VEC_FREE(VEC) do { \
    free((VEC)->buf); \
} while (0)


#define VEC_AT(VEC, POS) ((VEC)->buf[POS])
#define VEC_POP(VEC) ({ \
    typeof(*(VEC)->buf) _result; \
    if ((VEC)->len == 0) { \
        _result = (VEC)->buf[0]; \
        abort(); \
    } else { \
        _result = (VEC)->buf[--((VEC)->len)]; \
        if ((VEC)->len < (VEC)->cap / 3) \
            _VEC_RESIZE((VEC), (VEC)->cap / 2); \
    } \
    _result; \
})

#define VEC_PEEK(VEC) (&((VEC)->buf[(VEC)->len - 1]))
#define VEC_EMPTY(VEC) ((VEC)->len == 0)
#define VEC_LEN(VEC) ((VEC)->len)
#define VEC_CAP(VEC) ((VEC)->cap)
#define VEC_DEBUG(VEC, FMT) do { \
    int _i = 0; \
    for (; _i < (VEC)->len; _i++) { \
        if (_i > 0) \
            printf(" "); \
        printf(FMT, (VEC)->buf[_i]); \
    } \
    printf("\n"); \
} while(0)
#endif
