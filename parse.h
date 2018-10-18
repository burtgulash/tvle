#ifndef PARSE_H
#define PARSE_H

// TYPES
typedef struct Lexer *Lexer;

// TODO hide Token from public interface
typedef struct Token *Token;

#include "node.h"
#include "vec.h"

VEC_OF(Node)

// FUNCS
Lexer Lexer_new(char *str);
Node Lex_parse(Lexer lex, char **err);
Node parse(char *code, char **err);

int lexer_next(Lexer self);

// TODO hide Token from public interface
Token Token_from_lexer(Lexer);
void Token_print(Token tok);
#endif
