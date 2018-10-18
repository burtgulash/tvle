#ifndef NODE_H
#define NODE_H

typedef struct Node *Node;
typedef struct Debug *Debug;
//typedef union Value *Value;

#include "string.h"
#include "htable.h"
#include "stack.h"

typedef Node (*BuiltinFn)(Node, Node);
typedef Node (*SpecialFn)(Node, Node, Node*, Stack*);
typedef struct Function *Function;

// ENUMS
typedef enum {
    NT_LEAF,
    NT_TREE,
    NT_VALUE,
    NT_ENV,

    NT_QUOTE,
    NT_BLOCK,
    NT_MACRO,
} NT;

typedef enum {
    TT_NONE,
    TT_NUM,
    TT_SYMBOL,
    TT_STRING,
    TT_ESCAPED,
    TT_COMMENT,
    TT_PUNCTUATION,
    TT_CONS,
    TT_SEPARATOR,
    TT_NEWLINE,
    TT_SPACE,
    TT_LPAREN,
    TT_RPAREN,
} TT;

struct Debug {
    int l, h, r;
    char *source;
};

struct Cons {
    Node l, r;
};

struct Cont {
    Stack st;
    Node env;
};

struct Function {
    char *x, *y;
    Node body, env;
};

union Value {
    int i;
    double d;
    struct String s;
    struct Cons cons;
    struct Cont continuation;
    BuiltinFn builtin;
    SpecialFn special;
    Function function;
};

struct Node {
    int rc;
    NT nt;
    Debug debug;
    union {
        struct {
            TT tt;
            char *s;
            int len;
        } Leaf;
        struct {
            Node l, h, r;
        } Tree;
        struct {
            Node b;
        } Block;
        struct {
            Node p;
            union Value v;
        } Value;
        struct {
            Node p;
            HTable d;
        } Env;
    } nu;
};

// FUNCS
Node Node_new();
Node Node_new_env(Node p);
Node Node_new_value(Node p, union Value v);

// TODO owned or non owned s?
Node Node_new_leaf(TT tt, char *s, Debug debug);
Node Node_new_tree(Node l, Node h, Node r, Debug debug);
Node Node_forget(Node self);
Node Node_borrow(Node self);
Node Node_env_lookup(Node env, char *key);
Node Node_block(Node inner, NT node_type);

Function Function_new(char *x, char *y, Node body, Node env);

Debug Debug_copy(Debug debug);
Debug Debug_from_lhr(Node l, Node h, Node r);

#endif
