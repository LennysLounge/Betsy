#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "operation.h"
#include "array.h"
#include "iterator.h"
#include "expression.h"
#include "statement.h"

char *read_entire_file(char *filename)
{
    FILE *input = fopen(filename, "rb");
    if (input == NULL)
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

bool sntoi(char *word, int *out)
{
    int value = 0;
    int i = 0;
    while (word[i] != 0)
    {
        if (word[i] < 48 || word[i] > 57)
        {
            return false;
        }
        value = value * 10 + (word[i] - 48);
        i++;
    }
    *out = value;
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
        strncpy(token, file_text + iter.start, iter.length);
        token[iter.length] = 0;

        // Create location
        struct Location loc;
        loc.filename = filename;
        loc.line = iter.line + 1;
        loc.collumn = iter.start - iter.line_collumn_start + 1;

        // Create operation
        struct Operation op;
        int value;
        assert(INTRINSIC_TYPE_COUNT == 2 && "Exhaustive handling of intrinsic types");
        assert(KEYWORD_TYPE_COUNT == 2 && "Exhaustive handling of keyword types");
        // INTRINSICS
        if (strcmp(token, "print") == 0)
            op = OP_INTRINSIC_PRINT;
        else if (strcmp(token, "+") == 0)
            op = OP_INTRINSIC_PLUS;
        // KEYWORDS
        else if (strcmp(token, "if") == 0)
            op = OP_KEYWORD_IF;
        else if (strcmp(token, "var") == 0)
            op = OP_KEYWORD_VAR;
        // VALUES
        else if (sntoi(token, &value))
        {
            op = OP_VALUE_INT;
            op.value.value = value;
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
        fprintf(stderr, "%s:%d:%d ERROR: Expected an expression but got nothing.",
                prev_op->loc.filename, prev_op->loc.line, prev_op->loc.collumn + (int)strlen(prev_op->token));
        exit(1);
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
        case VALUE:
            values_available++;
            Array_add(&exp.operations, op);
            break;
        case INTRINSIC:
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
        case IDENTIFIER:
            struct Operation *id_op = get_identifier(identifiers, op->token);
            if (id_op == NULL)
            {
                fprintf(stderr, "%s:%d:%d ERROR: Unknown identifier '%s'\n",
                        op->loc.filename, op->loc.line, op->loc.collumn, op->token);
                exit(1);
            }
            values_available++;
            Array_add(&exp.operations, op);
            break;
        default:
            fprintf(stderr, "%s:%d:%d ERROR: The word '%s' is not a valid operation in an expression. Only intrinsics and values are allowed in expressions for now.\n",
                    op->loc.filename, op->loc.line, op->loc.collumn, op->token);
            exit(1);
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
        fprintf(stderr, "%s:%d:%d ERROR: Unfinished expression '%s'.\n",
                op->loc.filename, op->loc.line, op->loc.collumn, op->token);
        fprintf(stderr, "%s:%d:%d NOTE:  The operation '%s' takes %d input%s ",
                op->loc.filename, op->loc.line, op->loc.collumn, op->token,
                op->intrinsic.nr_inputs, op->intrinsic.nr_inputs > 1 ? "s" : "");
        int inputs_provided = op->intrinsic.nr_inputs - top_tuple->required_size + values_available;
        fprintf(stderr, "but %s%d %s provided.\n",
                inputs_provided > 0 ? "only " : "",
                inputs_provided,
                inputs_provided == 1 ? "was" : "were");
        // TODO: Maybe we can just log the error and continue.
        exit(1);
    }

    exp.nr_outputs = values_available;
    Array_free(&stack);
    return exp;
}

void parse_program(struct Array *program, struct Array *operations)
{
    struct Array identifiers;
    Array_init(&identifiers, sizeof(struct Operation));

    struct Iterator iter_ops = Iterator_create(operations);
    while (Iterator_hasNext(&iter_ops))
    {
        struct Operation *op = Iterator_peekNext(&iter_ops);
        assert(OPERATION_TYPE_COUNT == 4 && "Exhaustive handling of Operation types");
        struct Statement statement = {0};
        switch (op->type)
        {
        case KEYWORD:
            assert(KEYWORD_TYPE_COUNT == 2 && "Exhaustive handling of Keywords");
            switch (op->keyword.type)
            {
            case KEYWORD_TYPE_IF:
                Iterator_next(&iter_ops);

                statement.type = STATEMENT_TYPE_IF;
                statement.iff.condition = parse_expression(&iter_ops, &identifiers);
                statement.iff.action = parse_expression(&iter_ops, &identifiers);

                break;
            case KEYWORD_TYPE_VAR:
                Iterator_next(&iter_ops);

                if (!Iterator_hasNext(&iter_ops))
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Unexpected end of file.",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }

                statement.type = STATEMENT_TYPE_VAR;
                statement.var.identifier = *((struct Operation *)Iterator_next(&iter_ops));
                statement.var.assignment = parse_expression(&iter_ops, &identifiers);

                struct Operation *id_op = get_identifier(&identifiers, statement.var.identifier.token);
                if (id_op != NULL)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Redefinition of variable '%s'.\n",
                            op->loc.filename, op->loc.line, op->loc.collumn, statement.var.identifier.token);
                    fprintf(stderr, "%s:%d:%d NOTE:  The original variable was defined in %s:%d:%d\n",
                            op->loc.filename, op->loc.line, op->loc.collumn,
                            id_op->loc.filename, id_op->loc.line, id_op->loc.collumn);
                    exit(1);
                }

                Array_add(&identifiers, &statement.var.identifier);

                break;
            default:
                fprintf(stderr, "Unhandled keyword type '%d' in 'prase_program'\n", op->keyword.type);
                exit(1);
            }
            break;
        case IDENTIFIER:
            fprintf(stderr, "ERROR: We do not support calling variables like functions.\n");
            exit(1);
            break;
        default:
            // naked expression as statement
            statement.type = STATEMENT_TYPE_EXP;
            statement.expression = parse_expression(&iter_ops, &identifiers);
            break;
        }
        Array_add(program, &statement);
    }
    Array_free(&identifiers);
}

struct IdentifierElement
{
    struct Operation *identifier;
    size_t value;
};

struct IdentifierElement *get_identifier_element(struct Array *identifiers, char *id)
{
    for (int i = 0; i < identifiers->length; i++)
    {
        struct IdentifierElement *element = Array_get(identifiers, i);
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
        size_t a, b;
        //_Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        switch (op->type)
        {
        case INTRINSIC:
            switch (op->intrinsic.type)
            {
            case PRINT:
                if (outputs->length < 1)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the print intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                a = *((size_t *)Array_pop(outputs));
                printf("%zd\n", a);
                break;
            case PLUS:
                if (outputs->length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                a = *((size_t *)Array_pop(outputs));
                b = *((size_t *)Array_pop(outputs));
                b = a + b;
                Array_add(outputs, &b);
                break;
            }
            break;
        case VALUE:
            Array_add(outputs, &op->value.value);
            break;
        case IDENTIFIER:
            struct IdentifierElement *id_elem = get_identifier_element(identifiers, op->token);
            if (id_elem == NULL)
            {
                fprintf(stderr, "%s:%d:%d ERROR: Unknown identifier '%s'\n",
                        op->loc.filename, op->loc.line, op->loc.collumn, op->token);
                exit(1);
            }
            Array_add(outputs, &id_elem->value);
            break;
        default:
            fprintf(stderr, "ERROR: Operation of type '%d' not implemented yet in 'simulate_expression'",
                    op->type);
            exit(1);
        }
    }
}

void simulate_program(struct Array *program)
{
    struct Array exp_output;
    Array_init(&exp_output, sizeof(size_t));

    struct Array identifiers;
    Array_init(&identifiers, sizeof(struct IdentifierElement));

    for (int i = 0; i < program->length; i++)
    {
        struct Statement *statement = Array_get(program, i);
        switch (statement->type)
        {
        case STATEMENT_TYPE_EXP:
            simulate_expression(statement->expression, &exp_output, &identifiers);
            break;

        case STATEMENT_TYPE_IF:
            simulate_expression(statement->iff.condition, &exp_output, &identifiers);
            if (exp_output.length != 1)
            {
                struct Operation *op = Array_top(&statement->iff.condition.operations);
                fprintf(stderr, "%s:%d:%d ERROR: If condition must produce exactly one output.\n",
                        op->loc.filename, op->loc.line, op->loc.collumn);
                exit(1);
            }
            size_t result = *((size_t *)Array_get(&exp_output, 0));
            if (result == 0)
            {
                simulate_expression(statement->iff.action, &exp_output, &identifiers);
            }
            break;
        case STATEMENT_TYPE_VAR:
            struct IdentifierElement *prev_id = get_identifier_element(&identifiers, statement->var.identifier.token);
            if (prev_id != NULL)
            {
                struct Operation op = statement->var.identifier;
                fprintf(stderr, "%s:%d:%d ERROR: Redefinition of identifier '%s'.\n",
                        op.loc.filename, op.loc.line, op.loc.collumn, op.token);
                struct Operation *og_op = prev_id->identifier;
                fprintf(stderr, "%s:%d:%d NOTE:  Original definition was in %s:%d:%d.\n",
                        op.loc.filename, op.loc.line, op.loc.collumn,
                        og_op->loc.filename, og_op->loc.line, og_op->loc.collumn);
                exit(1);
            }
            // add identifier
            struct IdentifierElement id;
            id.identifier = &statement->var.identifier;
            simulate_expression(statement->var.assignment, &exp_output, &identifiers);
            if (exp_output.length != 1)
            {
                struct Operation *op = Array_top(&statement->var.assignment.operations);
                fprintf(stderr, "%s:%d:%d ERROR: Variable assignment must produce exactly one output.\n",
                        op->loc.filename, op->loc.line, op->loc.collumn);
                exit(1);
            }
            id.value = *((size_t *)Array_get(&exp_output, 0));
            Array_add(&identifiers, &id);
            break;
        default:

            fprintf(stderr, "ERROR: Statement type %d not implement yet in 'simulate_program'.\n", statement->type);
            exit(1);
        }
    }

    printf("printing all identifiers:\n");
    for (int i = 0; i < identifiers.length; i++)
    {
        struct IdentifierElement *id = Array_get(&identifiers, i);
        printf("%10s: %zd\n", id->identifier->token, id->value);
    }
    Array_free(&exp_output);
    Array_free(&identifiers);
}

void compile_expression(FILE *output, struct Expression exp, int *max_stack_size)
{
    int stack_size = 0;

    for (int j = 0; j < exp.operations.length; j++)
    {
        struct Operation *op = Array_get(&exp.operations, j);
        //_Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        switch (op->type)
        {
        case INTRINSIC:
            switch (op->intrinsic.type)
            {
            case PRINT:
                if (stack_size < 1)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the print intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                stack_size--;
                fprintf(output, "    printf(\"%%zd\\n\", stack_%03d);\n", stack_size);
                break;
            case PLUS:
                if (stack_size < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    exit(1);
                }
                stack_size--;
                fprintf(output, "    stack_%03d = stack_%03d + stack_%03d;\n", stack_size - 1, stack_size - 1, stack_size);
                break;
            }
            break;
        case VALUE:
            fprintf(output, "    %sstack_%03d = %d;\n",
                    (stack_size == *max_stack_size) ? "size_t " : "",
                    stack_size, op->value.value);
            stack_size++;
            break;
        case IDENTIFIER:
            fprintf(output, "    %sstack_%03d = %s;\n",
                    (stack_size == *max_stack_size) ? "size_t " : "",
                    stack_size, op->token);
            stack_size++;
            break;
        default:
            fprintf(stderr, "ERROR: Operation type '%d' is not yet implemented in 'compile_expression'.\n",
                    op->type);
            exit(1);
        }
        if (stack_size > *max_stack_size)
            *max_stack_size = stack_size;
    }
}

void compile_program(struct Array *program)
{
    FILE *output = fopen("out.c", "w");
    if (output == NULL)
    {
        fprintf(stderr, "ERROR: cannot open 'out.c' for writing\n");
        exit(1);
    }

    fprintf(output, "#include <stdio.h>\n");
    fprintf(output, "int main(int argc, char *argv[])\n");
    fprintf(output, "{\n");
    int maximum_stack_size = 0;
    for (int i = 0; i < program->length; i++)
    {
        struct Statement *statement = Array_get(program, i);

        switch (statement->type)
        {
        case STATEMENT_TYPE_EXP:
            struct Expression exp = statement->expression;
            compile_expression(output, exp, &maximum_stack_size);
            printf("\n");
            break;
        case STATEMENT_TYPE_IF:
            compile_expression(output, statement->iff.condition, &maximum_stack_size);
            fprintf(output, "    if (stack_000 == 0)\n");
            fprintf(output, "    {\n");
            compile_expression(output, statement->iff.action, &maximum_stack_size);
            fprintf(output, "    }\n\n");
            break;
        case STATEMENT_TYPE_VAR:
            compile_expression(output, statement->var.assignment, &maximum_stack_size);
            fprintf(output, "    size_t %s = stack_000;\n", statement->var.identifier.token);
            break;
        default:
            fprintf(stderr, "ERROR: Statement type '%d' not implemented yet in 'compile_program'\n", statement->type);
            exit(1);
        }
    }
    fprintf(output, "    return 0;\n");
    fprintf(output, "}\n");
compile_program_exit:
    fclose(output);
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
        case STATEMENT_TYPE_IF:
            printf("condition: ");
            for (int i = 0; i < statement->iff.condition.operations.length; i++)
            {
                struct Operation *op = Array_get(&statement->iff.condition.operations, i);
                printf("%s ", op->token);
            }
            printf("\t action: ");
            for (int i = 0; i < statement->iff.action.operations.length; i++)
            {
                struct Operation *op = Array_get(&statement->iff.action.operations, i);
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
