#ifndef OPERATION_H
#define OPERATION_H

#include <stdlib.h>

enum OperationType
{
    PRINT,
    PLUS,
    VALUE,
    OPERATION_TYPE_COUNT
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
    int nr_outputs;
    int nr_inputs;
    int value;
};

void Operation_free(struct Operation *op)
{
    free(op->token);
}

const struct Operation OP_PRINT = {.type = PRINT, .nr_inputs = 1, .nr_outputs = 0};
const struct Operation OP_PLUS = {.type = PLUS, .nr_inputs = 2, .nr_outputs = 1};
const struct Operation OP_INT = {.type = VALUE, .nr_inputs = 0, .nr_outputs = 1};

#endif
