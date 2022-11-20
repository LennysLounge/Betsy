#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "array.h"
#include "operation.h"
#include "iterator.h"
#include "typeInfo.h"

struct Expression
{
    struct Array operations;
    struct Array outputs;
    int nr_outputs;
};

void Expression_init(struct Expression *exp)
{
    Array_init(&exp->operations, sizeof(struct Operation));
    Array_init(&exp->outputs, sizeof(enum Type_info));
}

void Expression_free(struct Expression *exp)
{
    for (int i = 0; i < exp->operations.length; i++)
    {
        Operation_free(Array_get(&exp->operations, i));
    }
    Array_free(&exp->operations);
    Array_free(&exp->outputs);
}
#endif