#ifndef STACK_H
#define STACK_H

typedef struct Frame *Frame;
typedef struct Frame frame;
typedef struct Stack *Stack;

#include "vec.h"
#include "node.h"

typedef enum {
    INS_L,
    INS_H,
    INS_R,
    INS_FN,
    INS_RETURN,
} INST;

struct Frame {
    Node l, h, r, env;
    Debug debug;
    INST next;
};

VEC_OF(frame)

struct Stack {
    Stack parent;
    int tag;
    int rc;
    struct Vec_frame v;
};

Stack Stack_new(Stack parent, int initial_size);
void Stack_push(Stack self, Node l, Node h, Node r, Node env, Debug debug, INST next);
struct Frame Stack_pop(Stack *self);
Frame Stack_peek(Stack self);
int Stack_empty(Stack self);

Stack Stack_spush(Stack self, int tag);
Stack Stack_spop(Stack self, int tag);

Stack Stack_new(Stack parent, int tag);
Stack Stack_scopy(Stack parent, Stack self);
//Stack Stack_copy(Stack self);

void Stack_debug(Stack self);

#endif
