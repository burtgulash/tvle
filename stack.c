#include <assert.h>
#include "stack.h"
#include "node.h"

#define INITIAL_SIZE 2

Stack Stack_new(Stack parent, int tag) {
    Stack st = (Stack) malloc(sizeof(struct Stack));
    st->parent = parent;
    st->tag = tag;
    st->rc = 1;
    VEC_INIT(&st->v, INITIAL_SIZE);
    return st;
}

Stack Stack_copy(Stack self) {
    // TODO initial size of vector known precisely. Set it?
    Stack copy = Stack_new(self->parent, self->tag);
    for (int i = 0; i < VEC_LEN(&self->v); i++) {
        struct Frame *f = &VEC_AT(&self->v, i);
        struct Frame f_copy = {
            l: Node_borrow(f->l),
            h: Node_borrow(f->h),
            r: Node_borrow(f->r),
            debug: Debug_copy(f->debug),
            next: f->next
        };
        VEC_PUSH(&copy->v, f_copy);
        //VEC_PUSH(&copy->v, VEC_AT(&self->v, i));
    }
    return copy;
}

Stack Stack_scopy(Stack parent, Stack self) {
    Stack new_tip = Stack_copy(self);

    Stack copy = new_tip;
    for (self = self->parent; self != NULL; self=self->parent) {
        copy->parent = Stack_copy(self);
        copy = copy->parent;
    }
    copy->parent = parent;

    return new_tip;
}

Stack Stack_forget(Stack self) {
    self->rc --;
    if (self->rc == 0) {
        VEC_FREE(&self->v);
        free(self);
        return NULL;
    }
    return self;
}

Stack Stack_borrow(Stack self) {
    self->rc++;
    return self;
}


void Stack_push(Stack self, Node l, Node h, Node r, Node env, Debug debug, INST next) {
    assert (self != NULL);
    struct Frame f = {l, h, r, env, debug, next};
    VEC_PUSH(&self->v, f);
}

struct Frame Stack_pop(Stack *self) {
    for (;;) {
        assert(*self != NULL);

        if (!VEC_EMPTY(&(*self)->v)) {
            return VEC_POP(&(*self)->v);
        }

        Stack parent = (*self)->parent;
        Stack_forget(*self); // TODO enable
        *self = parent;
    }
}

Frame Stack_peek(Stack self) {
    if (VEC_EMPTY(&self->v))
        return NULL;
    return VEC_PEEK(&self->v);
}

Stack Stack_spush(Stack self, int tag) {
    Stack child = Stack_new(self, tag);
    child->parent = self;
    return child;
}

Stack Stack_spop(Stack self, int tag) {
    for (; self != NULL; self = self->parent)
        if (self->tag == tag)
            break;
    return self;
}

void Stack_debug(Stack self) {
    printf("STACK START\n");
    for (; self != NULL; self=self->parent) {
        printf("[%d items] %p -> %p\n", VEC_LEN(&self->v), (void*) self, (void*) self->parent);
        int len = VEC_LEN(&self->v);
        for (int i = 0; i < len; i++) {
            printf("  ins: %d\n", VEC_AT(&self->v, len - 1).next);
        }
    }
    printf("STACK END\n");
}
