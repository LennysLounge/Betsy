#ifndef OPERATION_H
#define OPERATION_H

#include <stdlib.h>

enum Operation_type
{
    OPERATION_TYPE_KEYWORD,
    OPERATION_TYPE_INTRINSIC,
    OPERATION_TYPE_VALUE,
    OPERATION_TYPE_IDENTIFIER,
    OPERATION_TYPE_COUNT
};

enum Intrinsic_type
{
    INTRINSIC_TYPE_PRINT,
    INTRINSIC_TYPE_PLUS,
    INTRINSIC_TYPE_MINUS,
    INTRINSIC_TYPE_GT,
    INTRINSIC_TYPE_COUNT
};

enum KeywordType
{
    KEYWORD_TYPE_IF,
    KEYWORD_TYPE_VAR,
    KEYWORD_TYPE_DO,
    KEYWORD_TYPE_END,
    KEYWORD_TYPE_SET,
    KEYWORD_TYPE_WHILE,
    KEYWORD_TYPE_COUNT
};

struct Location
{
    char *filename;
    int collumn;
    int line;
};

struct Operation
{
    struct Location loc;
    char *token;
    enum Operation_type type;
    union
    {
        struct
        {
            enum Intrinsic_type type;
            int nr_outputs;
            int nr_inputs;
        } intrinsic;
        struct
        {
            enum KeywordType type;
        } keyword;
        struct
        {
            int value;
        } value;
        struct
        {
            char *word;
        } identifier;
    };
};

void Operation_free(struct Operation *op)
{
    free(op->token);
}

const struct Operation OP_INTRINSIC_PRINT = {.type = OPERATION_TYPE_INTRINSIC, .intrinsic.type = INTRINSIC_TYPE_PRINT, .intrinsic.nr_inputs = 1, .intrinsic.nr_outputs = 0};
const struct Operation OP_INTRINSIC_PLUS = {.type = OPERATION_TYPE_INTRINSIC, .intrinsic.type = INTRINSIC_TYPE_PLUS, .intrinsic.nr_inputs = 2, .intrinsic.nr_outputs = 1};
const struct Operation OP_INTRINSIC_MINUS = {.type = OPERATION_TYPE_INTRINSIC, .intrinsic.type = INTRINSIC_TYPE_MINUS, .intrinsic.nr_inputs = 2, .intrinsic.nr_outputs = 1};
const struct Operation OP_INTRINSIC_GT = {.type = OPERATION_TYPE_INTRINSIC, .intrinsic.type = INTRINSIC_TYPE_GT, .intrinsic.nr_inputs = 2, .intrinsic.nr_outputs = 1};

const struct Operation OP_VALUE_INT = {.type = OPERATION_TYPE_VALUE, .value = 0};

const struct Operation OP_IDENTIFIER = {.type = OPERATION_TYPE_IDENTIFIER, .identifier.word = NULL};

const struct Operation OP_KEYWORD_IF = {.type = OPERATION_TYPE_KEYWORD, .keyword.type = KEYWORD_TYPE_IF};
const struct Operation OP_KEYWORD_VAR = {.type = OPERATION_TYPE_KEYWORD, .keyword.type = KEYWORD_TYPE_VAR};
const struct Operation OP_KEYWORD_DO = {.type = OPERATION_TYPE_KEYWORD, .keyword.type = KEYWORD_TYPE_DO};
const struct Operation OP_KEYWORD_END = {.type = OPERATION_TYPE_KEYWORD, .keyword.type = KEYWORD_TYPE_END};
const struct Operation OP_KEYWORD_SET = {.type = OPERATION_TYPE_KEYWORD, .keyword.type = KEYWORD_TYPE_SET};
const struct Operation OP_KEYWORD_WHILE = {.type = OPERATION_TYPE_KEYWORD, .keyword.type = KEYWORD_TYPE_WHILE};

#endif
