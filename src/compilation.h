#pragma once

#include "array.h"
#include "expression.h"
#include "statement.h"

#define com_error(location, ...)                                                                 \
    {                                                                                            \
        fprintf(stderr, "%s:%d:%d ERROR: ", location.filename, location.line, location.collumn); \
        fprintf(stderr, __VA_ARGS__);                                                            \
        exit(1);                                                                                 \
    }

#define fprintf_i(file, indent, ...)       \
    fprintf(file, "%*s", indent * 4, " "); \
    fprintf(file, __VA_ARGS__);

struct Com_identifier
{
    struct Operation *identifier;
    enum Type_info type;
};

struct Com_identifier *get_com_identifier(struct Array *identifiers, char *id)
{
    for (int i = 0; i < identifiers->length; i++)
    {
        struct Com_identifier *element = Array_get(identifiers, i);
        if (strcmp(element->identifier->token, id) == 0)
        {
            return element;
        }
    }
    return NULL;
}

void compile_expression(FILE *output, int indent, struct Expression exp, int *max_stack_size, struct Array *identifiers)
{
    struct Array type_info_stack;
    Array_init(&type_info_stack, sizeof(enum Type_info));

    for (int j = 0; j < exp.operations.length; j++)
    {
        struct Operation *op = Array_get(&exp.operations, j);
        //_Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        enum TypeInfo *r, *l;
        switch (op->type)
        {
        case OPERATION_TYPE_INTRINSIC:
            switch (op->intrinsic.type)
            {
            case INTRINSIC_TYPE_PRINT:
                if (type_info_stack.length < 1)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the print intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                enum Type_info *print_type = Array_pop(&type_info_stack);
                switch (*print_type)
                {
                case TYPE_INFO_INT:
                    fprintf_i(output, indent, "printf(\"%%d\\n\", (int32_t)stack_%03d);\n", type_info_stack.length);
                    break;
                default:
                    com_error(op->loc, "Print intrinsic not applicable for type %d.\n", *print_type);
                    break;
                }
                break;
            case INTRINSIC_TYPE_PLUS:
                if (type_info_stack.length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                // TODO: type check
                r = Array_pop(&type_info_stack);
                l = Array_pop(&type_info_stack);
                Array_add(&type_info_stack, l);
                fprintf_i(output, indent, "stack_%03d = stack_%03d + stack_%03d;\n",
                          type_info_stack.length - 1, type_info_stack.length - 1, type_info_stack.length);
                break;
            case INTRINSIC_TYPE_MINUS:
                if (type_info_stack.length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the minus intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                // TODO: type check
                r = Array_pop(&type_info_stack);
                l = Array_pop(&type_info_stack);
                Array_add(&type_info_stack, l);
                fprintf_i(output, indent, "stack_%03d = stack_%03d - stack_%03d;\n",
                          type_info_stack.length - 1, type_info_stack.length - 1, type_info_stack.length);
                break;
            case INTRINSIC_TYPE_GT:
                if (type_info_stack.length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the greater than intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                // TODO: type check
                r = Array_pop(&type_info_stack);
                l = Array_pop(&type_info_stack);
                Array_add(&type_info_stack, l);
                fprintf_i(output, indent, "stack_%03d = stack_%03d > stack_%03d;\n",
                          type_info_stack.length - 1, type_info_stack.length - 1, type_info_stack.length);
                break;
            case INTRINSIC_TYPE_MODULO:
                if (type_info_stack.length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the modulo intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                // TODO: type check
                r = Array_pop(&type_info_stack);
                l = Array_pop(&type_info_stack);
                Array_add(&type_info_stack, l);
                fprintf_i(output, indent, "stack_%03d = stack_%03d %% stack_%03d;\n",
                          type_info_stack.length - 1, type_info_stack.length - 1, type_info_stack.length);
                break;
            case INTRINSIC_TYPE_EQUAL:
                if (type_info_stack.length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the equal intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                // TODO: type check
                r = Array_pop(&type_info_stack);
                l = Array_pop(&type_info_stack);
                Array_add(&type_info_stack, l);
                fprintf_i(output, indent, "stack_%03d = stack_%03d == stack_%03d;\n",
                          type_info_stack.length - 1, type_info_stack.length - 1, type_info_stack.length);
                break;
            default:
                fprintf(stderr, "ERROR: Intrinsic of type '%d' is not yet implemented in 'compile_expression'.\n",
                        op->intrinsic.type);
                exit(1);
                break;
            }
            break;
        case OPERATION_TYPE_VALUE:
            fprintf_i(output, indent, "%sstack_%03d = %d;\n",
                      (type_info_stack.length == *max_stack_size) ? "uint64_t " : "",
                      type_info_stack.length, (int32_t)op->literal.value);
            Array_add(&type_info_stack, &op->literal.typeInfo);
            break;
        case OPERATION_TYPE_IDENTIFIER:
            fprintf_i(output, indent, "%sstack_%03d = %s;\n",
                      (type_info_stack.length == *max_stack_size) ? "uint64_t " : "",
                      type_info_stack.length, op->token);
            struct Com_identifier *id_id = get_com_identifier(identifiers, op->token);
            if (id_id == NULL)
                com_error(op->loc, "Unknown identifier '%s'.\n", op->token);
            Array_add(&type_info_stack, &id_id->type);
            break;
        default:
            fprintf(stderr, "ERROR: Operation type '%d' is not yet implemented in 'compile_expression'.\n",
                    op->type);
            exit(1);
        }
        if (type_info_stack.length > *max_stack_size)
            *max_stack_size = type_info_stack.length;
    }
    Array_free(&type_info_stack);
}

void compile_statement(FILE *output, int indent, struct Statement *statement, int *max_stack_size, struct Array *identifiers)
{
    switch (statement->type)
    {
    case STATEMENT_TYPE_EXP:
        struct Expression exp = statement->expression;
        compile_expression(output, indent, exp, max_stack_size, identifiers);
        break;
    case STATEMENT_TYPE_IF:
        compile_expression(output, indent, statement->iff.condition, max_stack_size, identifiers);
        fprintf_i(output, indent, "if (stack_000 != 0)\n");
        compile_statement(output, indent, statement->iff.action, max_stack_size, identifiers);
        break;
    case STATEMENT_TYPE_WHILE:
        fprintf_i(output, indent, "while (1)\n");
        fprintf_i(output, indent, "{\n");
        compile_expression(output, indent + 1, statement->whilee.condition, max_stack_size, identifiers);
        fprintf_i(output, (indent + 1), "if(stack_000 == 0)\n");
        fprintf_i(output, (indent + 2), "break;\n");
        compile_statement(output, indent + 1, statement->whilee.action, max_stack_size, identifiers);
        fprintf_i(output, indent, "}\n");
        break;
    case STATEMENT_TYPE_VAR:
        compile_expression(output, indent, statement->var.assignment, max_stack_size, identifiers);
        struct Com_identifier var_id;
        var_id.identifier = &statement->var.identifier;
        // TODO: get type from variable declaration
        var_id.type = TYPE_INFO_INT;
        Array_add(identifiers, &var_id);
        switch (var_id.type)
        {
        case TYPE_INFO_INT:
            fprintf_i(output, indent, "int32_t %s = stack_000;\n", statement->var.identifier.token);
            break;
        default:
            fprintf(stderr, "Type %d not implemented yet in 'compile_statement' 'STATEMENT_TYPE_VAR'.\n", var_id.type);
            exit(1);
        }
        break;
    case STATEMENT_TYPE_SET:
        compile_expression(output, indent, statement->set.assignment, max_stack_size, identifiers);
        struct Com_identifier *set_id = get_com_identifier(identifiers, statement->set.identifier.token);
        if (set_id == NULL)
            com_error(statement->set.identifier.loc, "Unknown identifier '%s'.\n", statement->set.identifier.token);
        switch (set_id->type)
        {
        case TYPE_INFO_INT:
            fprintf_i(output, indent, "%s = (int32_t)stack_000;\n", statement->set.identifier.token);
            break;
        default:
            fprintf(stderr, "Type %d not implemented yet in 'compile_statement' 'STATEMENT_TYPE_VAR'.\n", set_id->type);
            exit(1);
        }
        break;
    case STATEMENT_TYPE_BLOCK:
        fprintf_i(output, indent, "{\n");
        int prev_stack_size = *max_stack_size;
        int prev_identifier_length = identifiers->length;
        for (int i = 0; i < statement->block.statements.length; i++)
        {
            struct Statement *block_statement = Array_get(&statement->block.statements, i);
            compile_statement(output, indent + 1, block_statement, max_stack_size, identifiers);
        }
        fprintf_i(output, indent, "}\n");
        *max_stack_size = prev_stack_size;
        identifiers->length = prev_identifier_length;
        break;
    default:
        fprintf(stderr, "ERROR: Statement type '%d' not implemented yet in 'compile_program'\n", statement->type);
        exit(1);
    }
}

void compile_program(struct Array *program)
{
    FILE *output;
    if (fopen_s(&output, "out.c", "w"))
    {
        fprintf(stderr, "ERROR: cannot open 'out.c' for writing\n");
        exit(1);
    }

    struct Array identifiers;
    Array_init(&identifiers, sizeof(struct Com_identifier));

    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "#include <stdint.h>\n");
    fprintf(output, "#include <inttypes.h>\n");
    fprintf(output, "\n");
    fprintf(output, "int main(int argc, char *argv[])\n");
    fprintf(output, "{\n");
    int maximum_stack_size = 0;
    for (int i = 0; i < program->length; i++)
    {
        struct Statement *statement = Array_get(program, i);
        compile_statement(output, 1, statement, &maximum_stack_size, &identifiers);
    }
    fprintf(output, "    return 0;\n");
    fprintf(output, "}\n");
    fclose(output);

    Array_free(&identifiers);
    return;
}