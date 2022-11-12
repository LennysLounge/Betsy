#include <stdlib.h>
#include <stdio.h>

struct Tuple
{
    int index;
    int required_size;
};

struct Stack_tuple
{
    struct Tuple *start;
    int size;
    int capacity;
};

void Stack_tuple_init(struct Stack_tuple *array)
{
    const int initial_size = 8;
    array->start = malloc(sizeof(struct Tuple) * initial_size);
    array->size = 0;
    array->capacity = initial_size;
}

void Stack_tuple_free(struct Stack_tuple *array)
{
    free(array->start);
    array->start = 0;
    array->size = 0;
    array->capacity = 0;
}

void Stack_tuple_push(struct Stack_tuple *array, struct Tuple tuple)
{
    array->start[array->size] = tuple;
    array->size++;
    if (array->size == array->capacity)
    {
        array->start = realloc(array->start, sizeof(struct Tuple) * array->capacity * 2);
        array->capacity = array->capacity * 2;
    }
}

struct Tuple Stack_tuple_pop(struct Stack_tuple *array)
{
    if (array->size == 0)
    {
        fprintf(stderr, "ERROR: Stack is empty.");
        exit(1);
    }
    array->size--;
    return array->start[array->size];
}

struct Tuple Stack_tuple_peek(struct Stack_tuple *stack)
{
    if (stack->size == 0)
    {
        fprintf(stderr, "ERROR: Stack is empty.");
        exit(1);
    }
    return stack->start[stack->size - 1];
}
