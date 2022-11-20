#ifndef STATEMENT_H
#define STATEMENT_H

#include "expression.h"

enum Statement_type
{
    STATEMENT_TYPE_EXP,
    STATEMENT_TYPE_IF,
    STATEMENT_TYPE_WHILE,
    STATEMENT_TYPE_VAR,
    STATEMENT_TYPE_SET,
    STATEMENT_TYPE_BLOCK,
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
            struct Statement *action;
        } iff;
        struct
        {
            struct Expression condition;
            struct Statement *action;
        } whilee;
        struct
        {
            struct Operation identifier;
            struct Expression assignment;
            enum Type_info type_info;
        } var;
        struct
        {
            struct Operation identifier;
            struct Expression assignment;
        } set;
        struct
        {
            struct Array statements;
        } block;
    };
};

void Statement_free(struct Statement *statement)
{
    _Static_assert(STATEMENT_TYPE_COUNT == 6, "Exhaustive handling of statement types");
    switch (statement->type)
    {
    case STATEMENT_TYPE_EXP:
        Expression_free(&statement->expression);
        break;
    case STATEMENT_TYPE_IF:
        Expression_free(&statement->iff.condition);
        Statement_free(statement->iff.action);
        break;
    case STATEMENT_TYPE_WHILE:
        Expression_free(&statement->whilee.condition);
        Statement_free(statement->whilee.action);
        break;
    case STATEMENT_TYPE_VAR:
        Operation_free(&statement->var.identifier);
        Expression_free(&statement->var.assignment);
        break;
    case STATEMENT_TYPE_SET:
        Operation_free(&statement->set.identifier);
        Expression_free(&statement->set.assignment);
        break;
    case STATEMENT_TYPE_BLOCK:
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