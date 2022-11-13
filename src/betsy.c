#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "operation.h"
#include "stack_size_t.h"
#include "stack_tuple.h"

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

bool find_next_word(char *file_text, int start_pos, int *word_start, int *word_length, int *word_collumn, int *word_line)
{
    int pos = start_pos;
    int current_line_start = 0;
    // find start of the next word
    while (1)
    {
        if (file_text[pos] == 0)
        {
            *word_start = -1;
            *word_length = 0;
            return false;
        }
        else if (isgraph(file_text[pos]))
        {
            *word_start = pos;
            *word_collumn = pos - current_line_start;
            break;
        }
        else if (file_text[pos] == '\n')
        {
            (*word_line)++;
            current_line_start = pos;
        }
        pos++;
    }

    // find end of the current word
    while (isgraph(file_text[pos]))
        pos++;
    *word_length = pos - *word_start;
    return true;
}

bool sntoi(char *word, int length, int *out)
{
    int value = 0;
    int i = 0;
    while (word[i] != 0 && i < length)
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

void parse_file(struct OperationArray *operations, char *filename)
{
    char *file_text = read_entire_file(filename);

    int start = 0;
    int length = 0;
    int collumn = 0;
    int line = 0;
    while (find_next_word(file_text, start, &start, &length, &collumn, &line))
    {
        char *token = file_text + start;
        struct Location location = {.filename = filename, .collumn = collumn + 1, .line = line + 1};
        _Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of operation types");
        if (strncmp(token, "print", length) == 0)
        {
            OperationArray_add(operations, make_PRINT(location));
        }
        else if (strncmp(token, "+", length) == 0)
        {
            OperationArray_add(operations, make_PLUS(location));
        }
        else
        {
            int value = 0;
            if (sntoi(token, length, &value))
            {
                OperationArray_add(operations, make_INT(location, value));
            }
            else
            {
                fprintf(stderr, "%s:%d:%d ERROR: '%.*s' is not a valid word.",
                        location.filename, location.line, location.collumn, length, token);
                exit(1);
            }
        }
        start += length;
    }
    free(file_text);
}

void reverse_expression_order(struct OperationArray *operations)
{
    struct OperationArray new_ops;
    OperationArray_init(&new_ops);

    struct Stack_tuple expression_stack;
    Stack_tuple_init(&expression_stack);

    int values_available = 0;
    for (int i = 0; i < operations->size; i++)
    {
        struct Tuple t = {i, values_available + operations->start[i].nr_inputs};
        Stack_tuple_push(&expression_stack, t);

        struct Tuple current_op = t;
        while (values_available >= current_op.required_size)
        {
            struct Operation op = operations->start[current_op.index];
            OperationArray_add(&new_ops, op);
            values_available += op.nr_outputs - op.nr_inputs;
            Stack_tuple_pop(&expression_stack);
            if (expression_stack.size == 0)
            {
                if (values_available > 0)
                {
                    fprintf(stdout, "%s:%d:%d WARNING: Unhandled data after expression.\n",
                            op.loc.filename, op.loc.line, op.loc.collumn);
                }
                values_available = 0;
                break;
            }
            current_op = Stack_tuple_peek(&expression_stack);
        }
    }

    if (expression_stack.size != 0)
    {
        struct Operation op = operations->start[Stack_tuple_peek(&expression_stack).index];
        fprintf(stderr, "%s:%d:%d ERROR: Unfinished expression.\n",
                op.loc.filename, op.loc.line, op.loc.collumn);
        exit(1);
    }

    OperationArray_clear(operations);
    for (int i = 0; i < new_ops.size; i++)
    {
        OperationArray_add(operations, new_ops.start[i]);
    }
    OperationArray_free(&new_ops);
    Stack_tuple_free(&expression_stack);
}

void simulate_program(struct OperationArray *operations)
{
    struct Stack_size_t value_stack;
    Stack_size_t_init(&value_stack);

    for (int i = 0; i < operations->size; i++)
    {
        _Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        struct Operation op = operations->start[i];
        int a, b;
        switch (op.type)
        {
        case PRINT:
            a = Stack_size_t_pop(&value_stack);
            printf("%d\n", a);
            break;
        case PLUS:
            if (value_stack.size < 2)
            {
                fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                        op.loc.filename, op.loc.line, op.loc.collumn);
                goto simulate_program_exit;
            }
            a = Stack_size_t_pop(&value_stack);
            b = Stack_size_t_pop(&value_stack);
            Stack_size_t_push(&value_stack, a + b);
            break;
        case VALUE:
            Stack_size_t_push(&value_stack, op.value);
            break;
        }
    }
simulate_program_exit:
    Stack_size_t_free(&value_stack);
}

void compile_program(struct OperationArray *operations)
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
    int stack_size = 0;
    int maximum_stack_size = 0;
    for (int i = 0; i < operations->size; i++)
    {
        _Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        struct Operation op = operations->start[i];
        switch (op.type)
        {
        case PRINT:
            if (stack_size < 1)
            {
                fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the print intrinsic\n",
                        op.loc.filename, op.loc.line, op.loc.collumn);
                goto compile_program_exit;
            }
            stack_size--;
            fprintf(output, "    printf(\"%%d\\n\", stack_%03d);\n", stack_size);
            break;
        case PLUS:
            if (stack_size < 2)
            {
                fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                        op.loc.filename, op.loc.line, op.loc.collumn);
                goto compile_program_exit;
            }
            stack_size--;
            fprintf(output, "    stack_%03d = stack_%03d + stack_%03d;\n", stack_size - 1, stack_size - 1, stack_size);
            break;
        case VALUE:
            fprintf(output, "    %sstack_%03d = %d;\n",
                    (stack_size == maximum_stack_size) ? "int " : "",
                    stack_size, op.value);
            stack_size++;
            break;
        }
        if (stack_size > maximum_stack_size)
            maximum_stack_size = stack_size;
    }
    fprintf(output, "    return 0;\n");
    fprintf(output, "}\n");
compile_program_exit:
    return;
}

void print_usage(void)
{
    printf("Usage: betsy <subcommand> <filename>\n");
    printf("    Subcommands:\n");
    printf("        sim          : Simulate the program\n");
    printf("        com          : Compile the program\n");
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
    struct OperationArray operations;
    OperationArray_init(&operations);
    parse_file(&operations, argv[2]);
    reverse_expression_order(&operations);

    if (strcmp(subcommand, "sim") == 0)
    {
        simulate_program(&operations);
    }
    else if (strcmp(subcommand, "com") == 0)
    {
        compile_program(&operations);
    }
    else
    {
        fprintf(stderr, "ERROR: Unknown subcommand %s.\n", subcommand);
        print_usage();
    }
    OperationArray_free(&operations);
    return 0;
}
