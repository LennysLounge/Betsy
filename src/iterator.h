#ifndef ITERATOR_H
#define ITERATOR_H

#include <stdbool.h>

#include "array.h"

struct Iterator
{
    struct Array *array;
    int index;
};

struct Iterator Iterator_create(struct Array *array)
{
    struct Iterator iter;
    iter.array = array;
    iter.index = 0;
    return iter;
}

bool Iterator_hasNext(struct Iterator *iter)
{
    return iter->index < iter->array->length;
}

void *Iterator_next(struct Iterator *iter)
{
    if (!Iterator_hasNext(iter))
        return 0;
    return Array_get(iter->array, iter->index++);
}

void *Iterator_peekNext(struct Iterator *iter)
{
    if (!Iterator_hasNext(iter))
        return 0;
    return Array_get(iter->array, iter->index);
}

void *Iterator_peekNow(struct Iterator *iter)
{
    return Array_get(iter->array, iter->index);
}

#endif