#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "parse.h"
#include "htable.h"
#include "node.h"

char *PUNC_SET = ".:;|-$@&!%*+,?=<>/^~";

struct Lexer {
    char *str;
    int pos;
    int last;
    int c1;
    int c2;
    int backed;
    int done;
    TT state;
};

Lexer Lexer_new(char *str) {
    Lexer lex = (Lexer) malloc(sizeof(struct Lexer));
    lex->str = str;
    lex->pos = 0;
    lex->last = 0;
    lex->c1 = -1;
    lex->c2 = str[0];
    lex->state = TT_NONE;

    lex->backed = 0;
    lex->done = 0;
    return lex;
}

int lexer_forward(Lexer self) {
    self->pos++;
    self->c1 = self->c2;

    // only advance c2 if string has next char available
    int has_next = 0;
    if (self->str[self->pos - 1] == '\0') {
        assert(self->c1 == self->c2);
        assert(self->c1 == '\0');
    } else {
        has_next = 1;
        self->c2 = self->str[self->pos];
    }

    return has_next;
}

void lexer_backward(Lexer self) {
    self->pos--;
    self->c2 = self->c1;
    self->c1 = self->str[self->pos];
}

void lexer_return(Lexer self) {
    self->backed = 1;
}

// strchr matches even the \0 ending character of s, which is totally
// undesirable. Implement custom strchr
int strchr2(char *s, char c) {
    for (; *s; s++)
        if (*s == c)
            return 1;
    return 0;
}

int lexer_next(Lexer self) {
    if (self->backed == 1) {
        self->backed = 0;
        return self->pos;
    }

    if (self->done)
        return -1;


    self->last = self->pos;
    self->state = TT_NONE;

    for (;;) {
        if (self->c1 == '\0') {
            self->state = TT_RPAREN;
            self->done = 1;
            return self->pos;
        }
        lexer_forward(self);

        switch (self->state) {
            case TT_NONE:
                if (isdigit(self->c1)) {
                    self->state = TT_NUM;
                } else if (self->c1 == '_' && isdigit(self->c2)) {
                    self->state = TT_NUM;
                    lexer_forward(self);
                } else if (isalpha(self->c1)) {
                    self->state = TT_SYMBOL;
                } else if (self->c1 == '_' && isalpha(self->c2)) {
                    self->state = TT_SYMBOL;
                    lexer_forward(self);
                } else if (self->c1 == '_') {
                    self->state = TT_NUM;
                } else if (self->c1 == '"') {
                    self->state = TT_STRING;
                } else if (self->c1 == '\\') {
                    self->state = TT_ESCAPED;
                } else if (self->c1 == '#') {
                    self->state = TT_COMMENT;
                } else if (strchr2(PUNC_SET, self->c1)) {
                    self->state = TT_PUNCTUATION;
//                } else if (strchr2(".:", self->c1)) { // TODO remove
//                    self->state = TT_CONS;
//                    return self->pos;
//                } else if (strchr2(";|", self->c1)) {
//                    self->state = TT_SEPARATOR;
//                    return self->pos;
                } else if (strchr2("\n\r", self->c1)) {
                    self->state = TT_NEWLINE;
                } else if (strchr2(" \t", self->c1)) {
                    self->state = TT_SPACE;
                } else if (strchr2("([{", self->c1)) {
                    self->state = TT_LPAREN;
                    return self->pos;
                } else if (strchr2(")]}", self->c1)) {
                    self->state = TT_RPAREN;
                    return self->pos;
                }
                break;
            case TT_LPAREN:
            case TT_RPAREN:
            // case TT_CONS: // TODO remove
            //case TT_SEPARATOR:
                lexer_backward(self);
                return self->pos;
            case TT_NUM:
                if (!(self->c1 == '_' || isdigit(self->c1))) {
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_SYMBOL:
                if (!(self->c1 == '_' || isalnum(self->c1))) {
                    // TODO underscore as well
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_ESCAPED:
                if (strchr2("([{", self->c1)) {
                    self->state = TT_LPAREN;
                    return self->pos;
                } else if (!isalnum(self->c1)) {
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_STRING:
                // TODO handle escaped chars
                if (self->c1 == '"') {
                    //lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_COMMENT:
                if (self->c1 == '\0' || strchr2("\n\r", self->c1)) {
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_PUNCTUATION:
                if (!strchr2(PUNC_SET, self->c1)) {
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_SPACE:
                if (!strchr2(" \t", self->c1)) {
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            case TT_NEWLINE:
                if (!strchr2("\n\r", self->c1)) {
                    lexer_backward(self);
                    return self->pos;
                }
                break;
            default:
                break;
        }
    }
}

int lexer_next_filtered(Lexer self) {
    for (;;) {
        int pos = lexer_next(self);
        switch (self->state) {
            case TT_NONE:
            case TT_COMMENT:
            case TT_NEWLINE:
            case TT_SPACE:
                continue;

            default:
                return pos;
        }
    }
}


struct Token {
    TT tt;
    char *s;
    int len;
};

Debug Debug_new(int l, int h, int r, char *source) {
    Debug d = (Debug) malloc(sizeof(struct Debug));
    d->l = l;
    d->h = h;
    d->r = r;
    d->source = source;
    return d;
}

Debug Debug_from_lex(Lexer lex) {
    // TODO add source to lexer
    return Debug_new(lex->last, -1, lex->pos, NULL);
}

Debug Debug_from_lhr(Node l, Node h, Node r) {
    char *source = l->debug->source;
    return Debug_new(l->debug->l, h->debug->l, r->debug->r, source);
}

Debug Debug_copy(Debug d) {
    //assert(d != NULL); // TODO
    if (d == NULL)
        return NULL;
    Debug d2 = (Debug) malloc(sizeof(struct Debug));
    memcpy(d2, d, sizeof(struct Debug));
    return d2;
}

Node Node_from_lex(Lexer lex) {
    Node n = Node_new();
    n->nt = NT_LEAF;
    n->nu.Leaf.tt = lex->state;
    n->nu.Leaf.len = lex->pos - lex->last;
    n->nu.Leaf.s = strndup(&lex->str[lex->last], n->nu.Leaf.len);
    n->debug = Debug_from_lex(lex);
    return n;
}

Token Token_new(TT tt, char *buf, int from, int to) {
    Token l = (Token) malloc(sizeof(struct Token));
    l->tt = tt;
    l->len = to - from;
    l->s = strndup(&buf[from], l->len);
    return l;
}

Token Token_from_lexer(Lexer lex) {
    return Token_new(lex->state, lex->str, lex->last, lex->pos);
}

void Token_print(Token self) {
    if (self->tt == TT_RPAREN && self->s[0] == '\0')
        printf("[%s]%d\n", "$", self->tt);
    else
        printf("[%s]%d\n", self->s, self->tt);
}


// PARSING

char opposite_paren(char paren) {
    switch (paren) {
        case '(': return ')';
        case '{': return '}';
        case '[': return ']';
        //case '^': return '$';
        default: return '\0';
    }
}

Node lparse(Lexer lex, char end_paren, char **err);
Node rparse(Lexer lex, char end_paren, char **err);
Node subparse(Lexer lex, char end_paren, char **err);

Node subparse(Lexer lex, char end_paren, char **err) {
    int pos = lexer_next_filtered(lex);
    assert(pos != -1);

    Node n = Node_from_lex(lex);

    if (n->nu.Leaf.tt == TT_RPAREN) {
        // expected end_paren must match current RPAREN
        if (end_paren != n->nu.Leaf.s[0])
            *err = "subparse: parens don't match";

        // 'n' is just a matching RPAREN. get rid of it
        free(n);
        return NULL;
    } else if (n->nu.Leaf.tt == TT_LPAREN) {
        int is_quote = n->nu.Leaf.s[0] == '\\';
        char start_paren = n->nu.Leaf.s[is_quote ? 1 : 0];
        char end_paren = opposite_paren(start_paren);
        assert(end_paren != '\0');

        Node inner = lparse(lex, end_paren, err);
        if (*err != NULL)
            return inner;
        if (inner == NULL)
            // TODO just perform error checking. It will always
            // return value if no error
            assert(0);

        if (start_paren != '(')
            inner = Node_block(inner, is_quote ? NT_QUOTE : NT_BLOCK);

        // 'n' is just a LPAREN token, get rid of it
        free(n);
        return inner;
    }

    return n;
}

Node lparse(Lexer lex, char end_paren, char **err) {
    Node l = subparse(lex, end_paren, err);
    if (*err != NULL)
        return l;
    if (l == NULL)
        return Node_new_leaf(TT_NONE, strdup("()"), Debug_from_lex(lex));

    for (;;) {
        Node h = subparse(lex, end_paren, err);
        if (*err != NULL)
            return h;
        if (h == NULL)
            return l;

        // parse right leaning rest of tree until first end_paren
        //if (h->nt == NT_LEAF && h->nu.Leaf.tt == TT_SEPARATOR) { TODO remove
        if (h->nt == NT_LEAF && h->nu.Leaf.tt == TT_PUNCTUATION && strchr2(";|", h->nu.Leaf.s[0])) {
            Node r = lparse(lex, end_paren, err);
            if (*err != NULL)
                return r;

            // Block R and return composed tree
            return Node_new_tree(
                l,
                h,
                Node_block(r, NT_BLOCK),
                Debug_from_lhr(l, h, r)
            );
        }

        Node r = rparse(lex, end_paren, err);
        if (*err != NULL)
            return r;

        l = Node_new_tree(l, h, r, Debug_from_lhr(l, h, r));
    }
    return l;
}


Node rparse(Lexer lex, char end_token, char **err) {
    struct Vec_Node rights;
    VEC_INIT(&rights, 4);

    for (;;) {
        Node r = subparse(lex, end_token, err);
        if (*err != NULL)
            return r;
        if (r == NULL) {
            *err = "Invalid expression. R can't be ending token";
            return r;
        }

        VEC_PUSH(&rights, r);

        int pos = lexer_next_filtered(lex);
        assert(pos != -1);
        Node h = Node_from_lex(lex);

        if (!(h->nu.Leaf.tt == TT_PUNCTUATION && h->nu.Leaf.s[0] == ':')) {
        // if (h->nu.Leaf.tt != TT_CONS || h->nu.Leaf.s[0] != ':') { // TODO remove
            lexer_return(lex);
            break;
        }

        VEC_PUSH(&rights, h);
    }

    Node r = VEC_POP(&rights);
    while (!VEC_EMPTY(&rights)) {
        Node h = VEC_POP(&rights);
        Node l = VEC_POP(&rights);
        r = Node_new_tree(l, h, r, Debug_from_lhr(l, h, r));
    }

    return r;
}


Node Lex_parse(Lexer lex, char **err) {
    return lparse(lex, '\0', err);
}

Node parse(char *code, char **err) {
    Lexer lex = Lexer_new(code);
    Node parsed = Lex_parse(lex, err);
    free(lex); // TODO proper cleanup of lexer
    return parsed;
}
