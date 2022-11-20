#pragma once

#include <stdio.h>
#include <stdint.h>

#include "operation.h"
#include "expression.h"
#include "statement.h"

#define sim_error(location, ...)                                                                     \
    {                                                                                                \
        fprintf(stderr, "%s:%d:%d SIM_ERROR: ", location.filename, location.line, location.collumn); \
        fprintf(stderr, __VA_ARGS__);                                                                \
        exit(1);                                                                                     \
    }

struct Sim_value
{
    uint64_t data;
    enum Type_info type;
};

struct Sim_identifier
{
    struct Operation *identifier;
    struct Sim_value value;
};

struct Sim_identifier *get_sim_identifier(struct Array *identifiers, char *id)
{
    for (int i = 0; i < identifiers->length; i++)
    {
        struct Sim_identifier *element = Array_get(identifiers, i);
        if (strcmp(element->identifier->token, id) == 0)
        {
            return element;
        }
    }
    return NULL;
}

void simulate_expression(struct Expression exp, struct Array *outputs, struct Array *identifiers)
{
    outputs->length = 0;

    for (int j = 0; j < exp.operations.length; j++)
    {
        struct Operation *op = Array_get(&exp.operations, j);
        struct Sim_value *r, *l;
        //_Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        switch (op->type)
        {
        case OPERATION_TYPE_INTRINSIC:
            switch (op->intrinsic.type)
            {
            case INTRINSIC_TYPE_PRINT:
                if (outputs->length < 1)
                    sim_error(op->loc, "Not enough values for the print intrinsic.\n");
                struct Sim_value *print_value = Array_pop(outputs);
                switch (print_value->type)
                {
                case TYPE_INFO_INT:
                    printf("%d\n", (int32_t)print_value->data);
                    break;
                default:
                    sim_error(op->loc, "Print intrinsic not applicable for type %d.\n", print_value->type);
                    break;
                }
                break;
            case INTRINSIC_TYPE_PLUS:
                if (outputs->length < 2)
                    sim_error(op->loc, "Not enough values for the plus intrinsic.\n");
                r = Array_pop(outputs);
                l = Array_pop(outputs);
                // TODO: type check
                struct Sim_value plus_result = {
                    .data = l->data + r->data,
                    .type = r->type,
                };
                Array_add(outputs, &plus_result);
                break;
            case INTRINSIC_TYPE_MINUS:
                if (outputs->length < 2)
                    sim_error(op->loc, "Not enough values for the minus intrinsic.\n");
                r = Array_pop(outputs);
                l = Array_pop(outputs);
                // TODO: type check
                struct Sim_value minus_result = {
                    .data = l->data - r->data,
                    .type = r->type,
                };
                Array_add(outputs, &minus_result);
                break;
            case INTRINSIC_TYPE_GT:
                if (outputs->length < 2)
                    sim_error(op->loc, "Not enough values for the greater than intrinsic.\n");
                r = Array_pop(outputs);
                l = Array_pop(outputs);
                // TODO: type check
                struct Sim_value gt_result = {
                    .data = l->data > r->data,
                    .type = r->type,
                };
                Array_add(outputs, &gt_result);
                break;
            case INTRINSIC_TYPE_MODULO:
                if (outputs->length < 2)
                    sim_error(op->loc, "Not enough values for the modulo intrinsic.\n");
                r = Array_pop(outputs);
                l = Array_pop(outputs);
                // TODO: type check
                struct Sim_value modulo_result = {
                    .data = l->data % r->data,
                    .type = r->type,
                };
                Array_add(outputs, &modulo_result);
                break;
            case INTRINSIC_TYPE_EQUAL:
                if (outputs->length < 2)
                    sim_error(op->loc, "Not enough values for the equal intrinsic.\n");
                r = Array_pop(outputs);
                l = Array_pop(outputs);
                // TODO: type check
                struct Sim_value equal_result = {
                    .data = l->data == r->data,
                    .type = r->type,
                };
                Array_add(outputs, &equal_result);
                break;
            default:
                sim_error(op->loc, "Intrinsic of type '%d' not implemented yet in 'simulate_expression'", op->intrinsic.type);
                break;
            }
            break;
        case OPERATION_TYPE_VALUE:
            struct Sim_value value_result = {
                .data = op->literal.value,
                .type = op->literal.typeInfo,
            };
            Array_add(outputs, &value_result);
            break;
        case OPERATION_TYPE_IDENTIFIER:
            struct Sim_identifier *id_elem = get_sim_identifier(identifiers, op->token);
            if (id_elem == NULL)
                sim_error(op->loc, "Unknwon identifier '%s'.\n", op->token);
            Array_add(outputs, &id_elem->value);
            break;
        default:
            sim_error(op->loc, "Operation of type '%d' not implemented yet in 'simulate_expression'", op->type);
            break;
        }
    }
}

void simulate_statement(struct Statement *statement, struct Array *identifiers)
{
    struct Array exp_output;
    Array_init(&exp_output, sizeof(struct Sim_value));

    switch (statement->type)
    {
    case STATEMENT_TYPE_EXP:
        simulate_expression(statement->expression, &exp_output, identifiers);
        break;
    case STATEMENT_TYPE_IF:
        simulate_expression(statement->iff.condition, &exp_output, identifiers);
        if (exp_output.length != 1)
        {
            struct Operation *op = Array_top(&statement->iff.condition.operations);
            sim_error(op->loc, "If condition must produce exactly one output.\n");
        }
        struct Sim_value *if_result = Array_get(&exp_output, 0);
        //  TODO: do we have to cast to the correct type to check unequal zero? Maybe for floats or doubles?
        if (if_result->data != 0)
        {
            simulate_statement(statement->iff.action, identifiers);
        }
        break;
    case STATEMENT_TYPE_WHILE:
        while (true)
        {
            simulate_expression(statement->whilee.condition, &exp_output, identifiers);
            if (exp_output.length != 1)
            {
                struct Operation *op = Array_top(&statement->whilee.condition.operations);
                sim_error(op->loc, "While condition must produce exactly one output.\n");
            }
            struct Sim_value *while_result = Array_get(&exp_output, 0);
            // TODO: do we have to cast to the correct type to check unequal zero?
            if (while_result->data == 0)
                break;
            simulate_statement(statement->whilee.action, identifiers);
        }
        break;
    case STATEMENT_TYPE_VAR:
        struct Sim_identifier *prev_id = get_sim_identifier(identifiers, statement->var.identifier.token);
        if (prev_id != NULL)
            sim_error(statement->var.identifier.loc, "Variable '%s' was already defined here: %s:%d:%d.\n.",
                      statement->var.identifier.token,
                      prev_id->identifier->loc.filename,
                      prev_id->identifier->loc.line,
                      prev_id->identifier->loc.collumn);

        // add identifier
        struct Sim_identifier id;
        id.identifier = &statement->var.identifier;
        simulate_expression(statement->var.assignment, &exp_output, identifiers);
        if (exp_output.length != 1)
        {
            struct Operation *op = Array_top(&statement->var.assignment.operations);
            sim_error(op->loc, "Variable declaration must produce exactly one output.\n");
        }
        struct Sim_value *var_result = Array_get(&exp_output, 0);
        id.value = *var_result;
        Array_add(identifiers, &id);
        break;
    case STATEMENT_TYPE_SET:
        struct Sim_identifier *set_prev_id = get_sim_identifier(identifiers, statement->set.identifier.token);
        if (set_prev_id == NULL)
            sim_error(statement->var.identifier.loc, "Undefined variable '%s'.\n", statement->var.identifier.token);
        // evaluate expression
        simulate_expression(statement->set.assignment, &exp_output, identifiers);
        if (exp_output.length != 1)
        {
            struct Operation *op = Array_top(&statement->var.assignment.operations);
            sim_error(op->loc, "Variable declaration must produce exactly one output.\n");
        }
        struct Sim_value *set_result = Array_get(&exp_output, 0);
        set_prev_id->value = *set_result;
        break;
    case STATEMENT_TYPE_BLOCK:
        int identifiers_stack_length = identifiers->length;
        for (int i = 0; i < statement->block.statements.length; i++)
        {
            struct Statement *block_statement = Array_get(&statement->block.statements, i);
            simulate_statement(block_statement, identifiers);
        }
        identifiers->length = identifiers_stack_length;
        break;
    default:
        fprintf(stderr, "SIM_ERROR: Statement type %d not implement yet in 'simulate_statement'.\n", statement->type);
        exit(1);
    }
    Array_free(&exp_output);
}

void simulate_program(struct Array *program)
{
    struct Array identifiers;
    Array_init(&identifiers, sizeof(struct Sim_identifier));

    for (int i = 0; i < program->length; i++)
    {
        struct Statement *statement = Array_get(program, i);
        simulate_statement(statement, &identifiers);
    }

    Array_free(&identifiers);
}