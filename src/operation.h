#ifndef OPERATION_H
#define OPERATION_H

#include <stdlib.h>

enum OperationType
{
    KEYWORD,
    INTRINSIC,
    VALUE,
    OPERATION_TYPE_COUNT
};

enum IntrinsicType
{
    PRINT,
    PLUS,
    INTRINSIC_TYPE_COUNT
};

enum KeywordType
{
    KEYWORD_TYPE_IF,
    KEYWORD_TYPE_COUNT
};

struct Location
{
    char *filename;
    int collumn;
    int line;
};

struct Intrinsic
{
    enum IntrinsicType type;
    int nr_outputs;
    int nr_inputs;
};

struct Keyword
{
    enum KeywordType type;
};

struct Value
{
    int value;
};

struct Operation
{
    struct Location loc;
    char *token;
    enum OperationType type;
    union
    {
        struct Intrinsic intrinsic;
        struct Keyword keyword;
        struct Value value;
    };
};

void Operation_free(struct Operation *op)
{
    free(op->token);
}

const struct Operation OP_PRINT = {.type = INTRINSIC, .intrinsic.type = PRINT, .intrinsic.nr_inputs = 1, .intrinsic.nr_outputs = 0};
const struct Operation OP_PLUS = {.type = INTRINSIC, .intrinsic.type = PLUS, .intrinsic.nr_inputs = 2, .intrinsic.nr_outputs = 1};

const struct Operation VALUE_INT = {.type = VALUE, .value = 0};

const struct Operation KEYWORD_IF = {.type = KEYWORD, .keyword.type = KEYWORD_TYPE_IF};

#endif
