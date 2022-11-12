#include <stdlib.h>

struct Stack_size_t
{
    size_t *start;
    int size;
    int capacity;
};

void Stack_size_t_init(struct Stack_size_t *array)
{
    const int initial_size = 8;
    array->start = malloc(sizeof(size_t) * initial_size);
    array->size = 0;
    array->capacity = initial_size;
}

void Stack_size_t_free(struct Stack_size_t *array)
{
    free(array->start);
    array->start = 0;
    array->size = 0;
    array->capacity = 0;
}

void Stack_size_t_push(struct Stack_size_t *array, size_t n)
{
    array->start[array->size] = n;
    array->size++;
    if (array->size == array->capacity)
    {
        array->start = realloc(array->start, sizeof(size_t) * array->capacity * 2);
        array->capacity = array->capacity * 2;
    }
}

size_t Stack_size_t_pop(struct Stack_size_t *array)
{
    if (array->size == 0)
    {
        // TODO: There should be a failure condition for when an empty stack is popped.
        return 0;
    }
    array->size--;
    size_t v = array->start[array->size];
    array->start[array->size] = 0;
    return v;
}
