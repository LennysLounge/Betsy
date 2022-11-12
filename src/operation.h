#include <stdlib.h>

enum OperationType
{
    PRINT,
    PLUS,
    VALUE,
    OPERATION_TYPE_COUNT
};

struct Location
{
    char *filename;
    int collumn;
    int line;
};

struct Operation
{
    struct Location loc;
    enum OperationType type;
    int nr_outputs;
    int nr_inputs;
    int value;
};

struct OperationArray
{
    struct Operation *start;
    int size;
    int capacity;
};

void OperationArray_init(struct OperationArray *array)
{
    const int initial_size = 8;
    array->start = malloc(sizeof(struct Operation) * initial_size);
    array->size = 0;
    array->capacity = initial_size;
}

void OperationArray_free(struct OperationArray *array)
{
    free(array->start);
    array->start = 0;
    array->size = 0;
    array->capacity = 0;
}

void OperationArray_add(struct OperationArray *array, struct Operation operation)
{
    array->start[array->size] = operation;
    array->size++;
    if (array->size == array->capacity)
    {
        array->start = realloc(array->start, sizeof(struct Operation) * array->capacity * 2);
        array->capacity = array->capacity * 2;
    }
}

void OperationArray_clear(struct OperationArray *array)
{
    array->size = 0;
}

struct Operation make_INT(struct Location loc, int v)
{
    struct Operation op;
    op.loc = loc;
    op.type = VALUE;
    op.nr_inputs = 0;
    op.nr_outputs = 1;
    op.value = v;
    return op;
}

struct Operation make_PRINT(struct Location loc)
{
    struct Operation op;
    op.loc = loc;
    op.type = PRINT;
    op.nr_inputs = 1;
    op.nr_outputs = 0;
    return op;
}

struct Operation make_PLUS(struct Location loc)
{
    struct Operation op;
    op.loc = loc;
    op.type = PLUS;
    op.nr_inputs = 2;
    op.nr_outputs = 1;
    return op;
}