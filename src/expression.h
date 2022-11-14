#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "array.h"
#include "operation.h"
#include "iterator.h"

struct Expression
{
    struct Array operations;
    int nr_outputs;
};

void Expression_free(struct Expression *exp)
{
    for (int i = 0; i < exp->operations.length; i++)
    {
        Operation_free(Array_get(&exp->operations, i));
    }
}
#endif