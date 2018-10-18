#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "parse.h"
#include "node.h"
#include "string.h"
#include "htable.h"
#include "stack.h"

Node
    BUILTIN_P,
    STRING_P,
    NUM_P,
    CONS_P,
    FUNCTION_P,
    CONTINUATION_P,
    SPECIAL_P,
    NONE_P,
    BOOL_P,
    OBJ_P;

Node TVLE_NONE, TVLE_TRUE, TVLE_FALSE;
Node bu_shift(Node x, Node y, Node *env, Stack *st);
Node bu_cons(Node x, Node y);
void Node_debug(FILE *fp, int indent, Node self);

typedef enum {
    TAG_ZERO = 0,
    TAG_ERR = 1,
} TAG;

Node VS(char *s) {
    union Value v;
    String_init(&v.s, s, strlen(s));
    return Node_new_value(STRING_P, v);
}

Node VNUM(int i) {
    union Value v;
    v.i = i;
    return Node_new_value(NUM_P, v);
}

Node VFN(BuiltinFn fn) {
    union Value v;
    v.builtin = fn;
    return Node_new_value(BUILTIN_P, v);
}

Node VSP(SpecialFn fn) {
    union Value v;
    v.special = fn;
    return Node_new_value(SPECIAL_P, v);
}

Node Block(Node body) {
    Node b = Node_new();
    b->nt = NT_BLOCK;
    b->nu.Block.b = body; // TODO bump rc?
    return b;
}

Node Err(char *s) {
    return Node_new_tree(
        VNUM(TAG_ERR),
        VSP(bu_shift),
        Block(bu_cons(
            Node_borrow(TVLE_NONE),
            VS(s)
        )),
        NULL
    );
}

char* as_str(Node x) {
    if (x->nt == NT_LEAF) {
        if (x->nu.Leaf.tt == TT_SYMBOL)
            return strdup(x->nu.Leaf.s);
        else if (x->nu.Leaf.tt == TT_STRING)
            return strndup(x->nu.Leaf.s + 1, x->nu.Leaf.len - 2);
        else if (x->nu.Leaf.tt == TT_ESCAPED)
            return strndup(x->nu.Leaf.s + 1, x->nu.Leaf.len - 1);
    } if (x->nt == NT_VALUE && x->nu.Value.p == STRING_P) {
        return strdup(x->nu.Value.v.s.buf);
    }

    fprintf(stderr, "AsStr: CAN'T turn to string");
    abort();
}

Node bu_fn(Node x, Node y, Node *env, Stack *st) {
    char *x_name, *y_name;
    x_name = y_name = NULL;
    Node body = NULL;

    if (x->nt == NT_BLOCK) {
        body = x;
    } else if (x->nt == NT_VALUE && x->nu.Value.p == CONS_P) {
        Node args = x->nu.Value.v.cons.l;
        body = x->nu.Value.v.cons.r;
        if (body->nt != NT_BLOCK) {
            fprintf(stderr, "FN: 'body' must be a block");
            return Err("'body' mut be a block");
        }

        if (args->nt == NT_BLOCK)
            args = args->nu.Block.b;

        if (args->nt == NT_VALUE && args->nu.Value.p == CONS_P) {
            x_name = as_str(args->nu.Value.v.cons.l);
            y_name = as_str(args->nu.Value.v.cons.r);
        } else if (args->nt == NT_TREE) {
            x_name = as_str(args->nu.Tree.l);
            y_name = as_str(args->nu.Tree.r);
        } else {
            x_name = as_str(args);
            y_name = NULL;
        }
    }

    // TODO borrow env?
    Function fn = Function_new(x_name, y_name, body, Node_borrow(*env));
    union Value v;
    v.function = fn;
    return Node_new_value(FUNCTION_P, v);
}

Node bu_get_parent(Node x, Node y) {
    assert(x->nt == NT_ENV);
    return x->nu.Env.p;
}

Node bu_int_add(Node x, Node y) {
    if (x->nt != NT_VALUE || y->nt != NT_VALUE)
        return Err("Can't add non values.");
    if (x->nu.Value.p != NUM_P || y->nu.Value.p != NUM_P)
        return Err("Can't add non nums.");

    return VNUM(x->nu.Value.v.i + y->nu.Value.v.i);
}

Node bu_int_sub(Node x, Node y) {
    if (x->nt != NT_VALUE || y->nt != NT_VALUE)
        return Err("Can't sub non values.");
    if (x->nu.Value.p != NUM_P || y->nu.Value.p != NUM_P)
        return Err("Can't sub non nums.");

    return VNUM(x->nu.Value.v.i - y->nu.Value.v.i);
}

Node bu_int_mul(Node x, Node y) {
    if (x->nt != NT_VALUE || y->nt != NT_VALUE)
        return Err("Can't mul non values.");
    if (x->nu.Value.p != NUM_P || y->nu.Value.p != NUM_P)
        return Err("Can't mul non nums.");

    return VNUM(x->nu.Value.v.i * y->nu.Value.v.i);
}

Node bu_int_eq(Node x, Node y) {
    if (x->nt != NT_VALUE || y->nt != NT_VALUE)
        return Err("Can't eq non values.");
    if (x->nu.Value.p != NUM_P || y->nu.Value.p != NUM_P)
        return Err("Can't eq non nums.");

    if (x->nu.Value.v.i == y->nu.Value.v.i)
        return Node_borrow(TVLE_TRUE);
    return Node_borrow(TVLE_FALSE);
}

Node bu_isnone(Node x, Node y) {
    return x == TVLE_NONE ? Node_borrow(TVLE_TRUE) : Node_borrow(TVLE_FALSE);
}

Node bu_iscons(Node x, Node y) {
    return x->nt == NT_VALUE && x->nu.Value.p == CONS_P ? Node_borrow(TVLE_TRUE) : Node_borrow(TVLE_FALSE);
}

Node bu_cons(Node x, Node y) {
    struct Cons c = {Node_borrow(x), Node_borrow(y)};
    union Value v;
    v.cons = c;
    return Node_new_value(CONS_P, v);
}


Node bu_left(Node x, Node y) {
    assert (x->nt == NT_VALUE && x->nu.Value.p == CONS_P);
    return x->nu.Value.v.cons.l;
}

Node bu_right(Node x, Node y) {
    assert (x->nt == NT_VALUE && x->nu.Value.p == CONS_P);
    return x->nu.Value.v.cons.r;
}

Node bu_y(Node x, Node y) {
    assert(y->nt == NT_BLOCK);
    return y->nu.Block.b;;
}

Node bu_do(Node x, Node y) {
    assert(x->nt == NT_BLOCK);
    return x->nu.Block.b;;
}

Node bu_done(Node x, Node y) {
    assert(x->nt == NT_BLOCK);
    return Node_block(x->nu.Block.b, NT_MACRO);
}

Node bu_print(Node x, Node y) {
    Node_debug(stdout, 0, x);
    fflush(stdout);
    return x;
}


void init_builtins() {
    OBJ_P = Node_new_env(NULL);

    BUILTIN_P = Node_new_env(NULL);
    BUILTIN_P->nu.Env.p = OBJ_P;

    FUNCTION_P = Node_new_env(NULL);
    FUNCTION_P->nu.Env.p = OBJ_P;

    CONTINUATION_P = Node_new_env(NULL);
    CONTINUATION_P->nu.Env.p = OBJ_P;

    SPECIAL_P = Node_new_env(NULL);
    SPECIAL_P->nu.Env.p = OBJ_P;

    BOOL_P = Node_new_env(NULL);
    BOOL_P->nu.Env.p = OBJ_P;

    TVLE_TRUE = Node_new_env(BOOL_P);
    TVLE_FALSE = Node_new_env(BOOL_P);
    assert(TVLE_TRUE != TVLE_FALSE);

    NONE_P = Node_new_env(NULL);
    NONE_P->nu.Env.p = OBJ_P;
    TVLE_NONE = Node_new_env(NONE_P);

    NUM_P = Node_new_env(NULL);
    NUM_P->nu.Env.p = OBJ_P;
    HTable_set(NUM_P->nu.Env.d, "+", VFN(bu_int_add));
    HTable_set(NUM_P->nu.Env.d, "-", VFN(bu_int_sub));
    HTable_set(NUM_P->nu.Env.d, "*", VFN(bu_int_mul));
    HTable_set(NUM_P->nu.Env.d, "=", VFN(bu_int_eq));

    STRING_P = Node_new_env(NULL);
    STRING_P->nu.Env.p = OBJ_P;

    CONS_P = Node_new_env(NULL);
    CONS_P->nu.Env.p = OBJ_P;
}

// void free_builtins() {
//     Node_free(OBJ_P);
//     Node_free(BUILTIN_P);
//     Node_free(NUM_P);
//     Node_free(STRING_P);
//     Node_free(CONS_P);
// }


void Node_debug(FILE *fp, int indent, Node self) {
    if (self->nt == NT_LEAF) {
        char *leaf_type;
        switch (self->nu.Leaf.tt) {
            case TT_NONE: leaf_type = "NONE"; break;
            case TT_NUM: leaf_type = "NUM"; break;
            case TT_SYMBOL: leaf_type = "SYMBOL"; break;
            case TT_STRING: leaf_type = "STRING"; break;
            case TT_ESCAPED: leaf_type = "ESCAPED"; break;
            case TT_PUNCTUATION: leaf_type = "PUNC"; break;
            default: leaf_type = "OTHER"; break;
        }
        fprintf(fp, "%*s[Leaf[%s]:%s]\n", indent, "", leaf_type, self->nu.Leaf.s);
    } else if (self->nt == NT_BLOCK) {
        fprintf(fp, "%*s[%s]\n", indent, "", "BLOCK");
        Node_debug(fp, indent + 2, self->nu.Block.b);
    } else if (self->nt == NT_QUOTE) {
        fprintf(fp, "%*s[%s]\n", indent, "", "QUOTE");
        Node_debug(fp, indent + 2, self->nu.Block.b);
    } else if (self->nt == NT_TREE) {
        indent += 2;
        fprintf(fp, "%*s\n", indent, ".");
        Node_debug(fp, indent, self->nu.Tree.l);
        Node_debug(fp, indent, self->nu.Tree.h);
        Node_debug(fp, indent, self->nu.Tree.r);
        fprintf(fp, "%*s\n", indent, ".");
    } else if (self->nt == NT_VALUE) {
        if (self->nu.Value.p == NUM_P) {
            fprintf(fp, "%*s[Int:%d]\n", indent, "", self->nu.Value.v.i);
        } else if (self->nu.Value.p == BUILTIN_P) {
            fprintf(fp, "%*s[BuiltinFn]\n", indent, "");
        } else if (self->nu.Value.p == SPECIAL_P) {
            fprintf(fp, "%*s[SpecialFn]\n", indent, "");
        } else if (self->nu.Value.p == FUNCTION_P) {
            fprintf(fp, "%*s[Fn]\n", indent, "");
            Node_debug(fp, indent + 2, self->nu.Value.v.function->body);
        } else if (self->nu.Value.p == STRING_P) {
            fprintf(fp, "%*s[String:%s]\n", indent, "", self->nu.Value.v.s.buf);
        } else if (self->nu.Value.p == CONS_P) {
            indent += 2;
            Node_debug(fp, indent, self->nu.Value.v.cons.l);
            fprintf(fp, "%*s[Cons]\n", indent, "");
            Node_debug(fp, indent, self->nu.Value.v.cons.r);
        } else if (self->nu.Value.p == CONTINUATION_P) {
            fprintf(fp, "%*s[CONTINUATION]\n", indent, "");
        } else {
            fprintf(fp, "%*s[UNKNOWN VALUE]\n", indent, "");
        }
    } else if (self == TVLE_NONE) {
        fprintf(fp, "%*s[None]\n", indent, "");
    } else if (self->nt == NT_ENV) {
        if (self == TVLE_TRUE) {
            fprintf(fp, "%*s[True]\n", indent, "");
        } else if (self == TVLE_FALSE) {
            fprintf(fp, "%*s[True]\n", indent, "");
        }
        // if (self->nt.Env.p == BOOL_P) {
            // unreachable
        // }
    }
    fflush(fp);
    // TODO Env debug
}

Node bu_as(Node x, Node y, Node *env, Stack *st) {
    if (y->nt == NT_VALUE && y->nu.Value.p == CONS_P) {
        assert(x->nt == NT_VALUE && x->nu.Value.p == CONS_P);
        bu_as(x->nu.Value.v.cons.l, y->nu.Value.v.cons.l, env, st);
        bu_as(x->nu.Value.v.cons.r, y->nu.Value.v.cons.r, env, st);
    } else if (y->nt == NT_VALUE && y->nu.Value.p == STRING_P) {
        char *key = y->nu.Value.v.s.buf;
        HTable_set((*env)->nu.Env.d, key, Node_borrow(x));
    } else {
        assert(0);
    }
    return x;
}

Node bu_reset(Node x, Node y, Node *env, Stack *st) {
    assert(x->nt == NT_VALUE && x->nu.Value.p == NUM_P);
    assert(y->nt == NT_BLOCK);

    Node block_contents = y->nu.Block.b; // TODO ref decrease?
    int tag = x->nu.Value.v.i;

    *st = Stack_spush(*st, tag);
    return block_contents;
}

Node bu_shift(Node x, Node y, Node *env, Stack *st) {
    assert(x->nt == NT_VALUE && x->nu.Value.p == NUM_P);
    int tag = x->nu.Value.v.i;

    // Disallow shifting to 0 tag to prevent the root stack being eaten
    if (tag == TAG_ZERO)
        return Err("Can't shift to 0 tag");

    // TODO can abort. Check somehow?
    Stack head = *st;
    Stack tail = Stack_spop(*st, tag);

    // Popped with non matching tag, ended up at stack root, which is NULL
    if (tail == NULL) {
        // Restore stack with ZERO, RETURN, ERR
        *st = Stack_new(NULL, TAG_ZERO);
        Stack_push(*st, NULL, NULL, NULL, *env, NULL, INS_RETURN);
        *st = Stack_spush(*st, TAG_ERR);
        return Err("SHIFT non matching tag");
    }

    // Unlink continuation stack from parent stack
    *st = tail->parent;
    tail->parent = NULL;

    // Not possible to eat the ZERO TAG (0), because shift accepts only non
    // zero tags
    assert (*st != NULL);

    struct Cont k_ = {
        head, // Continuation takes ownership of head of stack
        Node_borrow(*env) // Env is only borrowed however
    };
    union Value v;
    v.continuation = k_;

    Node k = Node_new_value(CONTINUATION_P, v);

    // This will make use of y being a block, which will get turned
    // to a function.
    // Suppose y is a block '(\x|x + 3)', then the result will be:
    // k (\x|x + 3) ()
    y = Node_new_tree(k, y, Node_borrow(TVLE_NONE), Debug_copy(y->debug)); // TODO DEBUG FROM WHERE?

    return y;
}

Node eval(Node x, Node env, Stack st) {
    assert(env != NULL);
    assert(st != NULL);
    assert(env->nt == NT_ENV);

    char *tmp;
    struct Frame frame;
    Node l, h, r;
    l = h = r = NULL;

    Stack_push(st, l, h, r, env, NULL, INS_RETURN);

    for (;;) {
        // printf("Xaddr %d\n", x);
        Debug debug = x->debug;

        switch (x->nt) {
            case NT_VALUE:
            case NT_ENV:
            case NT_BLOCK:
            case NT_MACRO:
                frame = Stack_pop(&st);

                // Restore old items
                l = frame.l;
                h = frame.h;
                r = frame.r;

                Node inner_env = env;
                env = frame.env;
                debug = frame.debug;

                // Continue to point after calling eval recursively
                switch (frame.next) {
                    case INS_L: goto AFTER_L;
                    case INS_H: goto AFTER_H;
                    case INS_R: goto AFTER_R;
                    case INS_FN:
                        if (x->nt == NT_MACRO)
                            x = x->nu.Block.b; // TODO free macro?

                        // Clean up items in inner env? TODO
                        //Node_forget(inner_env); // TODO verify // Doesn't work with continuations
                        // Take result of fn application and continue evaluating it
                        break;
                    case INS_RETURN: return x;
                    default: abort();
                }
                break;

            case NT_QUOTE:
                x = x->nu.Block.b;
                break;

            case NT_TREE:
                l = x->nu.Tree.l;
                h = x->nu.Tree.h;
                r = x->nu.Tree.r;

                Stack_push(st, l, h, r, env, debug, INS_L);
                x = l; continue;
                AFTER_L: l = x;

                Stack_push(st, l, h, r, env, debug, INS_H);
                x = h; continue;
                AFTER_H: h = x;

                Stack_push(st, l, h, r, env, debug, INS_R);
                x = r; continue;
                AFTER_R: r = x;

                // TODO remove in final
                if (h == TVLE_NONE) {
                    x = h;
                    break;
                }

                // Implement conditionals
                if (h == TVLE_TRUE) {
                    x = l;
                    break;
                } else if (h == TVLE_FALSE) {
                    x = r;
                    break;
                }

                if (h->nt == NT_ENV) {
                    // TODO fix
                    x = l;
                    break;
                } else if (h->nt == NT_BLOCK) {
                    x = Node_new_tree(
                        l,
                        Node_new_tree(
                            h,
                            VS("fn"),
                            Node_borrow(TVLE_NONE),
                            Debug_copy(debug)
                        ),
                        r,
                        Debug_copy(debug)
                    );
                    break;
                }

                assert (h->nt == NT_VALUE);
                Node p = h->nu.Value.p;


                if (p == BUILTIN_P) {
                    BuiltinFn fn = h->nu.Value.v.builtin;
                    x = fn(l, r);
                    break;
                } else if (p == CONS_P) {
                    x = Node_new_tree(
                        l,
                        Node_new_tree(
                            h,
                            VS("fn"),
                            Node_borrow(TVLE_NONE),
                            Debug_copy(debug)
                        ),
                        r,
                        Debug_copy(debug)
                    );
                    break;
                } else if (p == SPECIAL_P) {
                    SpecialFn fn = h->nu.Value.v.special;
                    x = fn(l, r, &env, &st); // TODO borrow?
                    break;
                } else if (p == FUNCTION_P) {
                    Function fn = h->nu.Value.v.function;
                    Node closed_env = fn->env;

                    // Tail Call Optimization
                    Frame last_frame = Stack_peek(st);
                    // TODO test properly
                    if (last_frame == NULL || last_frame->h != h) {
                        // Save old items on stack
                        Stack_push(st, l, h, r, env, debug, INS_FN);

                        Node new_env = Node_new_env(closed_env);
                        env = new_env;
                    }

                    if (fn->x)
                        HTable_set(env->nu.Env.d, fn->x, l);
                    if (fn->y)
                        HTable_set(env->nu.Env.d, fn->y, r);
                    // TODO borrows

                    x = fn->body;
                    // Run Eval
                    continue;
                } else if (p == CONTINUATION_P) {
                    struct Cont k = h->nu.Value.v.continuation;
                    st = Stack_scopy(st, k.st);
                    x = l; env = k.env;
                    continue;
                } else if (p == STRING_P) {
                    //Node proto;
                    //if (l->nt == NT_ENV)
                    //    proto = l;
                    //else if (l->nt == NT_VALUE)
                    //    proto = l->nu.Value.p;

                    String fn_name = &h->nu.Value.v.s;
                    Node fn = Node_env_lookup(env, fn_name->buf);
                    // TODO increase fn.rc somewhere
                    if (!fn) {
                        x = Err("Method not found");
                        fprintf(stderr, "METHOD NOT FOUND %s\n", fn_name->buf);
                    } else {
                        x = Node_new_tree(l, fn, r, Debug_copy(debug));
                    }

                    break;
                } else {
                    fprintf(stderr, "Can't dispatch on this ENV %d\n", p);
                    x = Err("Can't dispatch on this ENV");
                }

                break;
                // TODO cleanup from branches above. Remove breaks and
                // continues and let them fall to this clean up section
                Node_forget(l);
                Node_forget(h);
                Node_forget(r);

                break;

            case NT_LEAF: ;
                union Value v;
                //fprintf(stderr, "Processing leaf: %d: '%s'\n", x->nu.Leaf.tt, x->nu.Leaf.s);

                switch (x->nu.Leaf.tt) {
                    case TT_NONE:
                        x = TVLE_NONE;
                        break;

                    case TT_NUM:
                        v.i = atoi(x->nu.Leaf.s);
                        // TODO errr check in parsing
                        x = Node_new_value(NUM_P, v);
                        break;

                    case TT_ESCAPED:
                        // strip start and end quotes from Leaf string token
                        String_init(&v.s, x->nu.Leaf.s + 1, x->nu.Leaf.len - 1);
                        x = Node_new_value(STRING_P, v);
                        break;

                    case TT_STRING:
                        // strip start and end quotes from Leaf string token
                        String_init(&v.s, x->nu.Leaf.s + 1, x->nu.Leaf.len - 2);
                        x = Node_new_value(STRING_P, v);
                        break;

                    case TT_SYMBOL:
                    case TT_PUNCTUATION:
                    //case TT_SEPARATOR:
                    //case TT_CONS: // TODO remove

                        tmp = x->nu.Leaf.s;
                        // TODO hack. Instead of allocation, set '\0' character temporarily

                        x = Node_env_lookup(env, tmp);
                        if (x == NULL) {
                            x = Err("Var not found");
                            fprintf(stderr, "VAR NOT FOUND: %s\n", tmp); // TODO
                        }

                        break;

                    default:
                        fprintf(stderr, "Can't process Leaf: %d\n", x->nu.Leaf.tt);
                        abort();
                }
                break;

            default:
                Node_debug(stdout, 0, x);
                fprintf(stderr, "Can't process Node: %d\n", x->nt);
                abort();
        }
    }
}

Node new_env() {
    HTable env_d = HTable_new(4);

    Node env = Node_new_env(NULL);
    env->nu.Env.d = env_d;

    return env;
}

void mod_register(Node mod, char *key, Node value) {
    assert(mod->nt == NT_ENV);
    HTable h = mod->nu.Env.d;
    HTable_set(h, key, value);
}

void tvle_open_base(Node mod) {
    mod_register(mod, "+", VFN(bu_int_add));
    mod_register(mod, "-", VFN(bu_int_sub));
    mod_register(mod, "*", VFN(bu_int_mul));
    mod_register(mod, "=", VFN(bu_int_eq));

    mod_register(mod, "isnone", VFN(bu_isnone));
    mod_register(mod, "iscons", VFN(bu_iscons));

    mod_register(mod, ".", VFN(bu_cons));
    mod_register(mod, ":", VFN(bu_cons));
    mod_register(mod, "|", VFN(bu_cons));
    mod_register(mod, ";", VFN(bu_y));
    mod_register(mod, "fn", VSP(bu_fn));
    mod_register(mod, "reset", VSP(bu_reset));
    mod_register(mod, "shift", VSP(bu_shift));
    mod_register(mod, "as", VSP(bu_as));
    mod_register(mod, "True", TVLE_TRUE);
    mod_register(mod, "False", TVLE_FALSE);
    mod_register(mod, "PR", VFN(bu_print));
    mod_register(mod, "l", VFN(bu_left));
    mod_register(mod, "r", VFN(bu_right));
    mod_register(mod, "do", VFN(bu_do));
    mod_register(mod, "done", VFN(bu_done));
}

Node execute(char *code, Node env, Stack st, char **err) {
    Node x = parse(code, err);
    if (*err != NULL)
        return NULL;

    Node err_handler = parse("\\x|x as \\result:\\error; (result) (error isnone.) (error)", err);
    if (*err != NULL)
        return NULL;

    //Print node after parsing
    //Node_debug(stdout, 0, x);

    // Add handler for shifts with TAG_ERR
    Stack_push(st, NULL, NULL, NULL, env, NULL, INS_RETURN);
    st = Stack_spush(st, TAG_ERR);

    // Mark happy path result as x:()
    x = Node_new_tree(x, VFN(bu_cons), Node_borrow(TVLE_NONE), x->debug);

    x = Node_borrow(x);
    Node y = eval(x, env, st);

    // TODO handle exceptions
    Node handle = Node_new_tree(
        y,
        err_handler,
        Node_borrow(TVLE_NONE),
        Debug_copy(y->debug)
    );

    y = eval(handle, env, st);


    // Clean up x

    return y;
}


int main(int argc, char **argv) {
    char *input_text;
    if (argc > 1)
        input_text = argv[1];
    else {
        //input_text = "3 + 4 ##\n#";
        input_text = "3 + 4";
    }
    printf("STR: %s\n", input_text);


//    Lexer lex = Lexer_new(input_text);
// //TEST LEXING
//    for (int i = 0;; i++) {
//        int pos = lexer_next(lex);
//        if (pos < 0)
//            break;
//        Token tok = Token_from_lexer(lex);
//        Token_print(tok);
//        free(tok);
//    }
//    return 0;

    init_builtins();

    Node env = new_env();
    tvle_open_base(env);

    // New stack
    Stack st = Stack_new(NULL, TAG_ZERO);

    char *err = NULL;

    // TODO catch errs in these snippets
    //execute("\\x.\\y.[x + y] as \"plus\"", env, st, &err);
    execute("(\\x.\\alt|x as \\conseq:\\cond; conseq cond alt done.) as \"||\"", env, st, &err);
    execute("(\\x.\\f|[()] : (x isnone()) || [x as \\l:\\r; (l map f) : (r map f)] : (x iscons()) || x f()) as \\map", env, st, &err);
    execute("(\\x.\\f|[x as \\l:\\r; (l fold f) f (r fold f)] : (x iscons()) || x) as \\fold", env, st, &err);
    execute("(\\x.\\f|[x as \\l:\\r; l fold0 f as \\lf; r fold0 f as \\rf; [lf] : (rf isnone()) || [rf] : (lf isnone()) || (lf) f (rf)] : (x iscons()) || x) as \\fold0", env, st, &err);
    execute("(\\x.\\y|x as \\item:\\var; item map (var.y fn.)) as \"|>\"", env, st, &err);
    execute("(\\header.\\body|header.body fn.) as \"::\"", env, st, &err);
    execute("\"::\" as \"|:\"", env, st, &err);
    execute("\"::\" as \".:\"", env, st, &err);
    // TODO macrodo

    Node x = execute(input_text, env, st, &err);

    if (err == NULL) {
        printf("\nRESULT:\n");
        Node_debug(stdout, 0, x);
    } else {
        fprintf(stderr, "ERR: %s\n", err);
    }
}
