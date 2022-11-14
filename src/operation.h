#ifndef OPERATION_H
#define OPERATION_H

#include <stdlib.h>

enum OperationType
{
    KEYWORD,
    INTRINSIC,
    VALUE,
    IDENTIFIER,
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
    KEYWORD_TYPE_VAR,
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
    enum OperationType type;
    union
    {
        struct
        {
            enum IntrinsicType type;
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

const struct Operation OP_INTRINSIC_PRINT = {.type = INTRINSIC, .intrinsic.type = PRINT, .intrinsic.nr_inputs = 1, .intrinsic.nr_outputs = 0};
const struct Operation OP_INTRINSIC_PLUS = {.type = INTRINSIC, .intrinsic.type = PLUS, .intrinsic.nr_inputs = 2, .intrinsic.nr_outputs = 1};

const struct Operation OP_VALUE_INT = {.type = VALUE, .value = 0};

const struct Operation OP_IDENTIFIER = {.type = IDENTIFIER, .identifier.word = NULL};

const struct Operation OP_KEYWORD_IF = {.type = KEYWORD, .keyword.type = KEYWORD_TYPE_IF};
const struct Operation OP_KEYWORD_VAR = {.type = KEYWORD, .keyword.type = KEYWORD_TYPE_VAR};

#endif
