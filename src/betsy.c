#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include "operation.h"
#include "array.h"
#include "iterator.h"
#include "expression.h"
#include "statement.h"

#include "simulation.h"
#include "compilation.h"

#define com_error(location, ...)                                                                 \
    {                                                                                            \
        fprintf(stderr, "%s:%d:%d ERROR: ", location.filename, location.line, location.collumn); \
        fprintf(stderr, __VA_ARGS__);                                                            \
        exit(1);                                                                                 \
    }

#define fprintf_i(file, indent, ...)       \
    fprintf(file, "%*s", indent * 4, " "); \
    fprintf(file, __VA_ARGS__);

char *read_entire_file(char *filename)
{
    FILE *input;
    if (fopen_s(&input, filename, "rb"))
    {
        fprintf(stderr, "ERROR: File '%s' cannot be opened.\n", filename);
        exit(0);
    }

    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    rewind(input);

    char *file_text = malloc(file_size + 1);
    fread(file_text, 1, file_size, input);
    file_text[file_size] = 0;
    fclose(input);

    return file_text;
}

struct FileIterator
{
    int start;
    int length;
    int line;
    int line_collumn_start;
};

bool find_next_word(char *file_text, struct FileIterator *iter)
{
    int pos = iter->start + iter->length;
    // find start of the next word
    while (1)
    {
        if (file_text[pos] == 0)
        {
            return false;
        }
        else if (file_text[pos] == '#')
        {
            while (file_text[pos] != 0 && file_text[pos] != '\n')
                pos++;
            pos--;
        }
        else if (isgraph(file_text[pos]))
        {
            iter->start = pos;
            break;
        }
        else if (file_text[pos] == '\n')
        {
            iter->line++;
            iter->line_collumn_start = pos + 1;
        }
        pos++;
    }

    // find end of the current word
    while (isgraph(file_text[pos]))
        pos++;
    iter->length = pos - iter->start;
    return true;
}

bool tryParseInteger(char *word, int32_t *out)
{
    int32_t i = 0;
    int32_t sign = 1;
    if (word[0] == '-')
    {
        sign = -1;
        i++;
    }
    int32_t value = 0;
    for (; word[i] != 0; i++)
    {
        if (word[i] == '_')
            continue;

        if (word[i] < '0' || word[i] > '9')
        {
            return false;
        }
        // Test overflow
        int8_t unit = word[i] - '0';
        if (value > INT32_MAX / 10 - unit)
            return false; // TODO: Give error message?
        value = value * 10 + unit;
    }
    *out = value * sign;
    return true;
}

void parse_file(struct Array *operations, char *filename)
{
    char *file_text = read_entire_file(filename);

    struct FileIterator iter = {0};
    while (find_next_word(file_text, &iter))
    {
        // Create null terminated token
        char *token = malloc(iter.length + 1);
        if (token == NULL)
        {
            fprintf(stderr, "ERROR: Allocation error in %s:%d\n", __FILE__, __LINE__);
            exit(1);
        }
        strncpy_s(token, iter.length + 1, file_text + iter.start, iter.length);
        token[iter.length] = 0;

        // Create location
        struct Location loc;
        loc.filename = filename;
        loc.line = iter.line + 1;
        loc.collumn = iter.start - iter.line_collumn_start + 1;

        // Create operation
        struct Operation op;
        int32_t value32;
        _Static_assert(INTRINSIC_TYPE_COUNT == 7, "Exhaustive handling of intrinsic types");
        _Static_assert(KEYWORD_TYPE_COUNT == 6, "Exhaustive handling of keyword types");
        // INTRINSICS
        if (strcmp(token, "print") == 0)
            op = OP_INTRINSIC_PRINT;
        else if (strcmp(token, "+") == 0)
            op = OP_INTRINSIC_PLUS;
        else if (strcmp(token, "-") == 0)
            op = OP_INTRINSIC_MINUS;
        else if (strcmp(token, ">") == 0)
            op = OP_INTRINSIC_GT;
        else if (strcmp(token, "%") == 0)
            op = OP_INTRINSIC_MODULO;
        else if (strcmp(token, "=") == 0)
            op = OP_INTRINSIC_EQUAL;
        else if (strcmp(token, "or") == 0)
            op = OP_INTRINSIC_OR;
        // KEYWORDS
        else if (strcmp(token, "if") == 0)
            op = OP_KEYWORD_IF;
        else if (strcmp(token, "var") == 0)
            op = OP_KEYWORD_VAR;
        else if (strcmp(token, "do") == 0)
            op = OP_KEYWORD_DO;
        else if (strcmp(token, "end") == 0)
            op = OP_KEYWORD_END;
        else if (strcmp(token, "set") == 0)
            op = OP_KEYWORD_SET;
        else if (strcmp(token, "while") == 0)
            op = OP_KEYWORD_WHILE;
        // VALUES
        else if (tryParseInteger(token, &value32))
        {
            op = OP_VALUE_INT;
            op.literal.value = value32;
        }
        else
        {
            op = OP_IDENTIFIER;
            op.identifier.word = op.token;
        }

        // Assign token and location to op.
        op.loc = loc;
        op.token = token;
        Array_add(operations, &op);
    }

    free(file_text);
}

struct Tuple
{
    struct Operation *op;
    int required_size;
};

struct Identifier
{
    struct Operation op;
    enum Type_info type_info;
};

struct Identifier *get_identifier(struct Array *array, char *name)
{
    for (int i = 0; i < array->length; i++)
    {
        struct Identifier *id = Array_get(array, i);
        if (strcmp(id->op.token, name) == 0)
        {
            return id;
        }
    }
    return NULL;
}

void parse_expression(struct Expression *exp, struct Iterator *operations_iter, struct Array *identifiers)
{
    if (!Iterator_hasNext(operations_iter))
    {
        struct Operation *prev_op = Array_get(operations_iter->array, operations_iter->index - 1);
        com_error(prev_op->loc, "Expected an expression but got nothing.\n");
    }

    struct Operation *op = Iterator_next(operations_iter);
    _Static_assert(OPERATION_TYPE_COUNT == 4, "Exhaustive handling of operation types");
    switch (op->type)
    {
    case OPERATION_TYPE_VALUE:
        Array_add(&exp->operations, op);
        Array_add(&exp->outputs, &op->literal.typeInfo);
        break;
    case OPERATION_TYPE_IDENTIFIER:
        struct Identifier *id_id = get_identifier(identifiers, op->token);
        if (id_id == NULL)
            com_error(op->loc, "Unkown identifier '%s'.\n", op->token);
        Array_add(&exp->operations, op);
        Array_add(&exp->outputs, &id_id->type_info);
        break;
    case OPERATION_TYPE_INTRINSIC:
        int prev_output_count = exp->outputs.length;
        _Static_assert(INTRINSIC_TYPE_COUNT == 7, "Exhaustive handling of intrinsic types");
        switch (op->intrinsic.type)
        {
        case INTRINSIC_TYPE_PRINT:
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 1)
                com_error(op->loc, "The 'print' intrinsic takes 1 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *print_i = Array_pop(&exp->outputs);
            if (*print_i == TYPE_INFO_INT || *print_i == TYPE_INFO_BOOL)
                Array_add(&exp->operations, op);
            else
                com_error(op->loc, "Cannot print values of type '%s'.\n", Type_info_name(*print_i));

            break;
        case INTRINSIC_TYPE_PLUS:
            parse_expression(exp, operations_iter, identifiers);
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 2)
                com_error(op->loc, "The 'plus' intrinsic takes 2 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *plus_r = Array_pop(&exp->outputs);
            enum TypeInfo *plus_l = Array_pop(&exp->outputs);
            if (*plus_r == TYPE_INFO_INT && *plus_r == *plus_l)
            {
                Array_add(&exp->operations, op);
                Array_add(&exp->outputs, plus_r);
            }
            else
                com_error(op->loc, "Cannot add values of type '%s' and '%s'.\n", Type_info_name(*plus_l), Type_info_name(*plus_r));
            break;
        case INTRINSIC_TYPE_MINUS:
            parse_expression(exp, operations_iter, identifiers);
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 2)
                com_error(op->loc, "The 'minus' intrinsic takes 2 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *minus_r = Array_pop(&exp->outputs);
            enum TypeInfo *minus_l = Array_pop(&exp->outputs);
            if (*minus_r == TYPE_INFO_INT && *minus_r == *minus_l)
            {
                Array_add(&exp->operations, op);
                Array_add(&exp->outputs, minus_r);
            }
            else
                com_error(op->loc, "Cannot subtract values of type '%s' and '%s'.\n", Type_info_name(*minus_l), Type_info_name(*minus_r));
            break;
        case INTRINSIC_TYPE_GT:
            parse_expression(exp, operations_iter, identifiers);
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 2)
                com_error(op->loc, "The 'greater than' intrinsic takes 2 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *gt_r = Array_pop(&exp->outputs);
            enum TypeInfo *gt_l = Array_pop(&exp->outputs);
            if (*gt_r == TYPE_INFO_INT && *gt_r == *gt_l)
            {
                Array_add(&exp->operations, op);
                enum Type_info gt_bool = TYPE_INFO_BOOL;
                Array_add(&exp->outputs, &gt_bool);
            }
            else
                com_error(op->loc, "Cannot compare values of type '%s' and '%s'.\n", Type_info_name(*gt_l), Type_info_name(*gt_r));
            break;
        case INTRINSIC_TYPE_MODULO:
            parse_expression(exp, operations_iter, identifiers);
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 2)
                com_error(op->loc, "The 'modulo' intrinsic takes 2 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *modulo_r = Array_pop(&exp->outputs);
            enum TypeInfo *modulo_l = Array_pop(&exp->outputs);
            if (*modulo_r == TYPE_INFO_INT && *modulo_r == *modulo_l)
            {
                Array_add(&exp->operations, op);
                Array_add(&exp->outputs, modulo_r);
            }
            else
                com_error(op->loc, "Cannot 'modulo' combine values of type '%s' and '%s'.\n", Type_info_name(*modulo_l), Type_info_name(*modulo_r));
            break;
        case INTRINSIC_TYPE_EQUAL:
            parse_expression(exp, operations_iter, identifiers);
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 2)
                com_error(op->loc, "The 'equal' intrinsic takes 2 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *equal_r = Array_pop(&exp->outputs);
            enum TypeInfo *equal_l = Array_pop(&exp->outputs);
            if (*equal_r == TYPE_INFO_INT && *equal_r == *equal_l)
            {
                Array_add(&exp->operations, op);
                enum Type_info equal_bool = TYPE_INFO_BOOL;
                Array_add(&exp->outputs, &equal_bool);
            }
            else
                com_error(op->loc, "Cannot compare values of type '%s' and '%s'.\n", Type_info_name(*equal_l), Type_info_name(*equal_r));
            break;
        case INTRINSIC_TYPE_OR:
            parse_expression(exp, operations_iter, identifiers);
            parse_expression(exp, operations_iter, identifiers);
            if (exp->outputs.length - prev_output_count != 2)
                com_error(op->loc, "The 'or' intrinsic takes 2 input but %d were provided.\n", exp->outputs.length);
            enum TypeInfo *or_r = Array_pop(&exp->outputs);
            enum TypeInfo *or_l = Array_pop(&exp->outputs);
            if (*or_r == TYPE_INFO_BOOL && *or_r == *or_l)
            {
                Array_add(&exp->operations, op);
                enum Type_info equal_bool = TYPE_INFO_BOOL;
                Array_add(&exp->outputs, &equal_bool);
            }
            else
                com_error(op->loc, "Cannot 'or' combine values of type '%s' and '%s'.\n", Type_info_name(*or_l), Type_info_name(*or_r));
            break;
        default:
            com_error(op->loc, "Intrinsic type '%d' is not implemented yet in 'parse_expression'.\n", op->intrinsic.type);
        }
        break;
    default:
        com_error(op->loc, "Operationt type '%d' not implemented yet in 'parse_expression'.\n", op->type);
    }
}

void parse_statement(struct Statement *statement, struct Iterator *iter_ops, struct Array *identifiers)
{
    struct Operation *op = Iterator_peekNext(iter_ops);
    _Static_assert(OPERATION_TYPE_COUNT == 4, "Exhaustive handling of Operation types");
    switch (op->type)
    {
    case OPERATION_TYPE_KEYWORD:
        _Static_assert(KEYWORD_TYPE_COUNT == 6, "Exhaustive handling of Keywords");
        switch (op->keyword.type)
        {
        case KEYWORD_TYPE_IF:
            Iterator_next(iter_ops);
            statement->type = STATEMENT_TYPE_IF;
            Expression_init(&statement->iff.condition);
            parse_expression(&statement->iff.condition, iter_ops, identifiers);
            struct Operation *if_op = Iterator_peekNext(iter_ops);
            if (if_op == NULL)
                com_error(op->loc, "Unexpected end of file.\n");
            if (if_op->type != OPERATION_TYPE_KEYWORD || if_op->keyword.type != KEYWORD_TYPE_DO)
                com_error(if_op->loc, "Unexpected word '%s' after if condition. Expected the start of a block.\n",
                          if_op->token);
            statement->iff.action = malloc(sizeof(struct Statement));
            parse_statement(statement->iff.action, iter_ops, identifiers);

            break;
        case KEYWORD_TYPE_WHILE:
            Iterator_next(iter_ops);
            statement->type = STATEMENT_TYPE_WHILE;
            Expression_init(&statement->whilee.condition);
            parse_expression(&statement->whilee.condition, iter_ops, identifiers);
            struct Operation *while_op = Iterator_peekNext(iter_ops);
            if (while_op == NULL)
                com_error(op->loc, "Unexpected end of file.\n");
            if (while_op->type != OPERATION_TYPE_KEYWORD || while_op->keyword.type != KEYWORD_TYPE_DO)
                com_error(while_op->loc, "Unexpected word '%s' after while condition. Expected the start of a block.\n",
                          while_op->token);
            statement->whilee.action = malloc(sizeof(struct Statement));
            parse_statement(statement->whilee.action, iter_ops, identifiers);
            break;
        case KEYWORD_TYPE_VAR:
            Iterator_next(iter_ops);

            // Parse identifier name
            if (!Iterator_hasNext(iter_ops))
                com_error(op->loc, "Unexpected end of file.\n");
            struct Operation *var_id_op = Iterator_next(iter_ops);

            // Check if the identifier is already declared
            struct Identifier *prev_var_id = get_identifier(identifiers, var_id_op->token);
            if (prev_var_id != NULL)
                com_error(op->loc, "Variable '%s' was already defined here: %s:%d:%d.\n.",
                          var_id_op->token, op->loc.filename, op->loc.line, op->loc.collumn);

            // Parse type info
            if (!Iterator_hasNext(iter_ops))
                com_error(op->loc, "Unexpected end of file.\n");
            struct Operation *var_type_op = Iterator_next(iter_ops);
            enum Type_info var_type = Type_info_by_name(var_type_op->token);
            if (var_type == -1)
                com_error(var_type_op->loc, "'%s' is not a valid type declaration.\n", var_type_op->token);

            // Add the identifier
            struct Identifier var_id = {
                .op = *var_id_op,
                .type_info = var_type,
            };
            Array_add(identifiers, &var_id);

            // Parse the expression
            struct Expression var_exp;
            Expression_init(&var_exp);
            parse_expression(&var_exp, iter_ops, identifiers);

            // Typecheck the expression
            if (var_exp.outputs.length != 1)
                com_error(var_id_op->loc, "Variable declaration must produce exactly one ouput.\n");

            enum Type_info *var_output = Array_top(&var_exp.outputs);
            if (*var_output != var_id.type_info)
                com_error(var_id_op->loc, "Variable '%s' is of type '%s' but the assignment is of type '%s'.\n",
                          var_id.op.token, Type_info_name(var_id.type_info), Type_info_name(*var_output));

            statement->type = STATEMENT_TYPE_VAR;
            statement->var.identifier = *var_id_op;
            statement->var.type_info = var_type;
            statement->var.assignment = var_exp;

            break;
        case KEYWORD_TYPE_SET:
            Iterator_next(iter_ops);

            if (!Iterator_hasNext(iter_ops))
                com_error(op->loc, "Unexpected end of file.\n");

            statement->type = STATEMENT_TYPE_SET;
            statement->set.identifier = *((struct Operation *)Iterator_next(iter_ops));

            // Check if the identifier is declared
            struct Identifier *set_id = get_identifier(identifiers, statement->set.identifier.token);
            if (set_id == NULL)
                com_error(op->loc, "Undefined variable '%s'.\n.", statement->set.identifier.token);

            // Parse expression
            Expression_init(&statement->set.assignment);
            parse_expression(&statement->set.assignment, iter_ops, identifiers);

            // Typecheck expression
            if (statement->set.assignment.outputs.length != 1)
                com_error(statement->set.identifier.loc, "Variable assignment must produce exactly one ouput.\n");

            enum Type_info *set_output = Array_top(&statement->set.assignment.outputs);
            if (*set_output != set_id->type_info)
                com_error(statement->set.identifier.loc, "Variable '%s' is of type '%s' but the assignment is of type '%s'.\n",
                          set_id->op.token, Type_info_name(set_id->type_info), Type_info_name(*set_output));
            break;
        case KEYWORD_TYPE_DO:
            Iterator_next(iter_ops);
            int identifier_stack_length = identifiers->length;

            statement->type = STATEMENT_TYPE_BLOCK;
            Array_init(&statement->block.statements, sizeof(struct Statement));

            struct Operation *block_op = Iterator_peekNext(iter_ops);
            while (block_op->type != OPERATION_TYPE_KEYWORD || block_op->keyword.type != KEYWORD_TYPE_END)
            {
                struct Statement block_statement;
                parse_statement(&block_statement, iter_ops, identifiers);
                Array_add(&statement->block.statements, &block_statement);

                if (!Iterator_hasNext(iter_ops))
                    com_error(block_op->loc, "Missing 'end' for 'do' in %s:%d:%d.\n",
                              op->loc.filename, op->loc.line, op->loc.collumn);

                block_op = Iterator_peekNext(iter_ops);
            }
            Iterator_next(iter_ops);
            identifiers->length = identifier_stack_length;
            break;
        case KEYWORD_TYPE_END:
            Iterator_next(iter_ops);
            com_error(op->loc, "Encountered 'end' without a matching 'do'.\n");
            break;

        default:
            fprintf(stderr, "Unhandled keyword type '%d' in 'prase_program'\n", op->keyword.type);
            exit(1);
        }
        break;
    case OPERATION_TYPE_IDENTIFIER:
        com_error(op->loc, "Unknown intrinsic '%s'. We do not support calling variable like functions yet.\n",
                  op->token);
        break;
    default:
        // naked expression as statement
        statement->type = STATEMENT_TYPE_EXP;
        Expression_init(&statement->expression);
        parse_expression(&statement->expression, iter_ops, identifiers);
        break;
    }
}

void parse_program(struct Array *program, struct Array *operations)
{
    struct Array identifiers;
    Array_init(&identifiers, sizeof(struct Identifier));

    struct Iterator iter_ops = Iterator_create(operations);
    while (Iterator_hasNext(&iter_ops))
    {
        struct Statement statement = {0};
        parse_statement(&statement, &iter_ops, &identifiers);
        Array_add(program, &statement);
    }
    Array_free(&identifiers);
}

void print_usage(void)
{
    printf("Usage: betsy <subcommand> <filename>\n");
    printf("    Subcommands:\n");
    printf("        sim          : Simulate the program\n");
    printf("        com          : Compile the program\n");
}

void print_program(struct Array *program)
{
    for (int j = 0; j < program->length; j++)
    {
        struct Statement *statement = Array_get(program, j);
        printf("%2d: type:%d | ", j, statement->type);

        switch (statement->type)
        {
        case STATEMENT_TYPE_EXP:
            printf("out:%d\t", statement->expression.nr_outputs);
            for (int i = 0; i < statement->expression.operations.length; i++)
            {
                struct Operation *op = Array_get(&statement->expression.operations, i);
                printf("%s ", op->token);
            }
            printf("\n");
            break;
        default:
            printf("ERROR: Statement type %d not implemented yet.\n", statement->type);
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "ERROR: No subcommand given\n");
        print_usage();
        return 0;
    }
    char *subcommand = argv[1];

    if (argc < 3)
    {
        fprintf(stderr, "ERROR: No filename given.\n");
    }

    struct Array operations;
    Array_init(&operations, sizeof(struct Operation));

    parse_file(&operations, argv[2]);

    struct Array program;
    Array_init(&program, sizeof(struct Statement));

    parse_program(&program, &operations);

    if (strcmp(subcommand, "sim") == 0)
    {
        simulate_program(&program);
    }
    else if (strcmp(subcommand, "com") == 0)
    {
        compile_program(&program);
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown subcommand %s.\n", subcommand);
        print_usage();
    }

    // print_program(&program);
    for (int i = 0; i < program.length; i++)
    {
        struct Statement *statement = Array_get(&program, i);
        Statement_free(statement);
    }
    Array_free(&program);
    Array_free(&operations);
    return 0;
}
