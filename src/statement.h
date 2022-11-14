#ifndef STATEMENT_H
#define STATEMENT_H

#include "expression.h"

enum Statement_type
{
    STATEMENT_TYPE_EXP,
    STATEMENT_TYPE_IF,
    STATEMENT_TYPE_COUNT,
};

struct Statement
{
    enum Statement_type type;
    union
    {
        struct Expression expression;
        struct
        {
            struct Expression condition;
            struct Expression action;
        } iff;
    };
};

void Statement_free(struct Statement *statement)
{
    switch (statement->type)
    {
    case STATEMENT_TYPE_EXP:
        Expression_free(&statement->expression);
        break;
    case STATEMENT_TYPE_IF:
        Expression_free(&statement->iff.condition);
        Expression_free(&statement->iff.action);
        break;
    default:
        fprintf(stderr, "Unhandle statement type '%d' in 'Statement_free'.\n", statement->type);
        exit(1);
    }
}

#endif