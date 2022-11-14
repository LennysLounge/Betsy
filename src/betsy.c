#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "operation.h"
#include "array.h"
#include "iterator.h"
#include "expression.h"

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
            exit(0);
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
        if (strcmp(token, "print") == 0)
            op = OP_PRINT;
        else if (strcmp(token, "+") == 0)
            op = OP_PLUS;
        else if (sntoi(token, &value))
        {
            op = OP_INT;
            op.value = value;
        }
        else
        {
            fprintf(stderr, "%s:%d:%d ERROR: '%s' is not a valid word.",
                    loc.filename, loc.line, loc.collumn, token);
            exit(1);
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

struct Expression parse_expression(struct Iterator *operations_iter)
{
    struct Expression exp;
    Array_init(&exp.operations, sizeof(struct Operation));

    struct Array stack;
    Array_init(&stack, sizeof(struct Tuple));

    int values_available = 0;
    while (Iterator_hasNext(operations_iter))
    {
        int i = operations_iter->index;
        struct Operation *op = (struct Operation *)Iterator_next(operations_iter);
        struct Tuple t = {i, values_available + op->nr_inputs};
        Array_add(&stack, &t);

        struct Tuple current_op = t;
        while (values_available >= current_op.required_size)
        {
            struct Operation *stack_op = Array_get(operations_iter->array, current_op.index);
            Array_add(&exp.operations, stack_op);

            values_available += stack_op->nr_outputs - stack_op->nr_inputs;
            Array_pop(&stack);
            if (stack.length == 0)
                goto expression_finished;
            current_op = *((struct Tuple *)Array_top(&stack));
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
                op->nr_inputs, op->nr_inputs > 1 ? "s" : "");
        int inputs_provided = op->nr_inputs - top_tuple->required_size + values_available;
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
    struct Iterator iter_ops = Iterator_create(operations);
    while (Iterator_hasNext(&iter_ops))
    {
        struct Expression exp = parse_expression(&iter_ops);
        Array_add(program, &exp);
    }
}

void simulate_program(struct Array *program)
{
    struct Array stack;
    Array_init(&stack, sizeof(int));

    for (int i = 0; i < program->length; i++)
    {
        struct Expression *exp = Array_get(program, i);
        stack.length = 0;

        for (int j = 0; j < exp->operations.length; j++)
        {
            struct Operation *op = Array_get(&exp->operations, j);
            int a, b;
            _Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
            switch (op->type)
            {
            case PRINT:
                if (stack.length < 1)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the print intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    goto simulate_program_exit;
                }
                a = *((int *)Array_pop(&stack));
                printf("%d\n", a);
                break;
            case PLUS:
                if (stack.length < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    goto simulate_program_exit;
                }
                a = *((int *)Array_pop(&stack));
                b = *((int *)Array_pop(&stack));
                b = a + b;
                Array_add(&stack, &b);
                break;
            case VALUE:
                Array_add(&stack, &op->value);
                break;
            }
        }
    }
simulate_program_exit:
    Array_free(&stack);
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
    int stack_size = 0;
    int maximum_stack_size = 0;
    for (int i = 0; i < program->length; i++)
    {
        struct Expression *exp = Array_get(program, i);
        stack_size = 0;

        fprintf(output, "\n");

        for (int j = 0; j < exp->operations.length; j++)
        {
            struct Operation *op = Array_get(&exp->operations, j);
            _Static_assert(OPERATION_TYPE_COUNT == 3, "Exhaustive handling of Operations");
            switch (op->type)
            {
            case PRINT:
                if (stack_size < 1)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the print intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    goto compile_program_exit;
                }
                stack_size--;
                fprintf(output, "    printf(\"%%d\\n\", stack_%03d);\n", stack_size);
                break;
            case PLUS:
                if (stack_size < 2)
                {
                    fprintf(stderr, "%s:%d:%d ERROR: Not enough values for the plus intrinsic\n",
                            op->loc.filename, op->loc.line, op->loc.collumn);
                    goto compile_program_exit;
                }
                stack_size--;
                fprintf(output, "    stack_%03d = stack_%03d + stack_%03d;\n", stack_size - 1, stack_size - 1, stack_size);
                break;
            case VALUE:
                fprintf(output, "    %sstack_%03d = %d;\n",
                        (stack_size == maximum_stack_size) ? "int " : "",
                        stack_size, op->value);
                stack_size++;
                break;
            }
            if (stack_size > maximum_stack_size)
                maximum_stack_size = stack_size;
        }
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

void print_program(struct Array *program)
{
    for (int j = 0; j < program->length; j++)
    {
        struct Expression *exp = Array_get(program, j);
        printf("out:%d\t", exp->nr_outputs);
        for (int i = 0; i < exp->operations.length; i++)
        {
            struct Operation *op = Array_get(&exp->operations, i);
            printf("%s ", op->token);
        }
        printf("\n");
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
    Array_init(&program, sizeof(struct Expression));

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
        struct Expression *exp = Array_get(&program, i);
        Expression_free(exp);
    }
    Array_free(&program);
    Array_free(&operations);
    return 0;
}
