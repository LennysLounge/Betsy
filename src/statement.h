#ifndef STATEMENT_H
#define STATEMENT_H

#include "expression.h"

enum Statement_type
{
    STATEMENT_TYPE_EXP,
    STATEMENT_TYPE_IF,
    STATEMENT_TYPE_VAR,
    STATEMENT_TYPE_BLOCk,
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
        struct
        {
            struct Operation identifier;
            struct Expression assignment;
        } var;
        struct
        {
            struct Array statements;
        } block;
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
    case STATEMENT_TYPE_VAR:
        Operation_free(&statement->var.identifier);
        Expression_free(&statement->var.assignment);
        break;
    case STATEMENT_TYPE_BLOCk:
        for (int i = 0; i < statement->block.statements.length; i++)
        {
            Statement_free(Array_get(&statement->block.statements, i));
        }
        break;
    default:
        fprintf(stderr, "Unhandle statement type '%d' in 'Statement_free'.\n", statement->type);
        exit(1);
    }
}

#endif