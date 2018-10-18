#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "node.h"
#include "htable.h"


Node Node_new() {
    Node n = (Node) malloc(sizeof(struct Node));
    n->rc = 1;
    n->nt = NT_QUOTE;
    return n;
}

Node Node_borrow(Node self) {
    self->rc += 1;
    return self;
}

Node Node_forget(Node self) {
    self->rc -= 1;
    if (self->rc <= 0) {
        assert(self->rc == 0);
        free(self);
        return NULL;
    }
    return self;
}

Node Node_block(Node inner, NT node_type) {
    Node b = Node_new();
    b->nu.Block.b = inner;
    b->debug = Debug_copy(inner->debug);
    b->nt = node_type;
    return b;
}


Node Node_new_tree(Node l, Node h, Node r, Debug debug) {
    Node n = Node_new();
    n->nt = NT_TREE;
    n->nu.Tree.l = l;
    n->nu.Tree.r = r;
    n->nu.Tree.h = h;
    n->debug = debug;
    return n;
}

Node Node_new_leaf(TT tt, char *s, Debug debug) {
    int len = strlen(s);

    Node n = Node_new();
    n->nt = NT_LEAF;
    n->nu.Leaf.tt = tt;
    n->nu.Leaf.s = strndup(s, len);
    n->nu.Leaf.len = len;
    n->debug = debug;
    return n;
}

Node Node_new_env(Node p) {
    Node n = Node_new();
    n->nt = NT_ENV;
    n->nu.Env.p = p;
    n->nu.Env.d = HTable_new(2);
    return n;
}

Node Node_new_value(Node p, union Value v) {
    Node n = Node_new();
    n->nt = NT_VALUE;
    n->nu.Value.p = p;
    n->nu.Value.v = v;
    return n;
}

Function Function_new(char *x, char *y, Node body, Node env) {
    assert(body->nt == NT_BLOCK);
    Function fn = (Function) malloc(sizeof(struct Function));
    fn->x = x;
    fn->y = y;

    // TODO borrow these
    fn->body = body->nu.Block.b;
    fn->env = env;

    return fn;
}

Node Node_env_lookup(Node env, char *key) {
    for (; env != NULL; env = env->nu.Env.p) {
        assert(env->nt == NT_ENV);

        Node v = HTable_get(env->nu.Env.d, key);
        if (v != NULL)
            return v;
    }

    return NULL;
}
