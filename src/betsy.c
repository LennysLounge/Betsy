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

#define com_error(location, ...)                                                                 \
    {                                                                                            \
        fprintf(stderr, "%s:%d:%d ERROR: ", location.filename, location.line, location.collumn); \
        fprintf(stderr, __VA_ARGS__);                                                            \
        exit(1);                                                                                 \
    }
#define sim_error(location, ...)                                                                     \
    {                                                                                                \
        fprintf(stderr, "%s:%d:%d SIM_ERROR: ", location.filename, location.line, location.collumn); \
        fprintf(stderr, __VA_ARGS__);                                                                \
        exit(1);                                                                                     \
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
        _Static_assert(INTRINSIC_TYPE_COUNT == 6, "Exhaustive handling of intrinsic types");
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
    int index;
    int required_size;
};

struct Operation *get_identifier(struct Array *array, char *name)
{
    for (int i = 0; i < array->length; i++)
    {
        struct Operation *op = Array_get(array, i);
        if (strcmp(op->token, name) == 0)
        {
            return op;
        }
    }
    return NULL;
}

struct Expression parse_expression(struct Iterator *operations_iter, struct Array *identifiers)
{
    if (!Iterator_hasNext(operations_iter))
    {
        struct Operation *prev_op = Array_get(operations_iter->array, operations_iter->index - 1);
        com_error(prev_op->loc, "Expected an expression but got nothing.\n");
    }

    struct Expression exp;
    Array_init(&exp.operations, sizeof(struct Operation));

    struct Array stack;
    Array_init(&stack, sizeof(struct Tuple));

    int values_available = 0;
    while (Iterator_hasNext(operations_iter))
    {
        int i = operations_iter->index;
        struct Operation *op = (struct Operation *)Iterator_next(operations_iter);

        switch (op->type)
        {
        case OPERATION_TYPE_VALUE:
            values_available++;
            Array_add(&exp.operations, op);
            break;
        case OPERATION_TYPE_INTRINSIC:
            if (op->intrinsic.nr_inputs == 0)
            {
                values_available += op->intrinsic.nr_outputs;
                Array_add(&exp.operations, op);
            }
            else
            {
                struct Tuple t = {i, values_available + op->intrinsic.nr_inputs};
                Array_add(&stack, &t);
            }
            break;
        case OPERATION_TYPE_IDENTIFIER:
            struct Operation *id_op = get_identifier(identifiers, op->token);
            if (id_op == NULL)
                com_error(op->loc, "Unkown identifier '%s'.\n", op->token);

            values_available++;
            Array_add(&exp.operations, op);
            break;
        default:
            com_error(op->loc, "The word '%s' is not a valid operation in an expression. Only intrinsics, values and identifiers are allowed for now.",
                      op->token);
        }
        if (stack.length == 0)
        {
            break;
        }
        struct Tuple *current_op = Array_top(&stack);
        while (values_available >= current_op->required_size)
        {
            struct Operation *stack_op = Array_get(operations_iter->array, current_op->index);
            Array_add(&exp.operations, stack_op);

            values_available += stack_op->intrinsic.nr_outputs - stack_op->intrinsic.nr_inputs;
            Array_pop(&stack);
            if (stack.length == 0)
                goto expression_finished;
            current_op = Array_top(&stack);
        }
    }
expression_finished:
    if (stack.length != 0)
    {
        struct Tuple *top_tuple = (struct Tuple *)Array_top(&stack);
        struct Operation *op = (struct Operation *)Array_get(operations_iter->array, top_tuple->index);
        int inputs_provided = op->intrinsic.nr_inputs - top_tuple->required_size + values_available;
        com_error(op->loc, "Unfinished expression. '%s' take %d input%s but %s%d %s provided.\n",
                  op->token,
                  op->intrinsic.nr_inputs,
                  op->intrinsic.nr_inputs > 1 ? "s" : "",
                  inputs_provided > 0 ? "only " : "",
                  inputs_provided,
                  inputs_provided == 1 ? "was" : "were");
    }

    exp.nr_outputs = values_available;
    Array_free(&stack);
    return exp;
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
            statement->iff.condition = parse_expression(iter_ops, identifiers);
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
            statement->whilee.condition = parse_expression(iter_ops, identifiers);
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

            if (!Iterator_hasNext(iter_ops))
                com_error(op->loc, "Unexpected end of file.\n");

            statement->type = STATEMENT_TYPE_VAR;
            statement->var.identifier = *((struct Operation *)Iterator_next(iter_ops));
            statement->var.assignment = parse_expression(iter_ops, identifiers);

            struct Operation *var_op = get_identifier(identifiers, statement->var.identifier.token);
            if (var_op != NULL)
                com_error(op->loc, "Variable '%s' was already defined here: %s:%d:%d.\n.",
                          statement->var.identifier.token, op->loc.filename, op->loc.line, op->loc.collumn);

            Array_add(identifiers, &statement->var.identifier);

            break;
        case KEYWORD_TYPE_SET:
            Iterator_next(iter_ops);

            if (!Iterator_hasNext(iter_ops))
                com_error(op->loc, "Unexpected end of file.\n");

            statement->type = STATEMENT_TYPE_SET;
            statement->set.identifier = *((struct Operation *)Iterator_next(iter_ops));
            statement->set.assignment = parse_expression(iter_ops, identifiers);

            struct Operation *set_op = get_identifier(identifiers, statement->set.identifier.token);
            if (set_op == NULL)
                com_error(op->loc, "Undefined variable '%s'.\n.", statement->set.identifier.token);

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
        statement->expression = parse_expression(iter_ops, identifiers);
        break;
    }
}

void parse_program(struct Array *program, struct Array *operations)
{
    struct Array identifiers;
    Array_init(&identifiers, sizeof(struct Operation));

    struct Iterator iter_ops = Iterator_create(operations);
    while (Iterator_hasNext(&iter_ops))
    {
        struct Statement statement = {0};
        parse_statement(&statement, &iter_ops, &identifiers);
        Array_add(program, &statement);
    }
    Array_free(&identifiers);
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
