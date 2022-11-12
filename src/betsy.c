#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#include "Op.h"
#include "stack.h"

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

void simulate_program(struct OperationArray *operations)
{
    struct Stack value_stack;
    Stack_init(&value_stack);
    for (int i = 0; i < operations->size; i++)
    {
        _Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
        struct Operation op = operations->start[i];
        int a, b;
        switch (op.type)
        {
        case PRINT:
            a = Stack_pop(&value_stack);
            printf("%d\n", a);
            break;
        case PLUS:
            if (value_stack.size < 2)
            {
                fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                        op.loc.filename, op.loc.line, op.loc.collumn);
                goto simulate_program_exit;
            }
            a = Stack_pop(&value_stack);
            b = Stack_pop(&value_stack);
            Stack_push(&value_stack, a + b);
            break;
        case VALUE:
            Stack_push(&value_stack, op.value);
            break;
        }
    }
simulate_program_exit:
    Stack_free(&value_stack);
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
