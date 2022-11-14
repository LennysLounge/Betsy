#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct Array
{
    char *data;
    int length;
    int capacity;
    int element_size;
};

void Array_init(struct Array *array, int element_size)
{
    char *data = malloc(element_size * 8);
    if (data == NULL)
    {
        fprintf(stderr, "ERROR: Array allocation error.\n");
        exit(1);
    }
    array->data = data;
    array->length = 0;
    array->capacity = 8;
    array->element_size = element_size;
}

void Array_free(struct Array *array)
{
    free(array->data);
}

void Array_add(struct Array *array, void *thing)
{
    if (array->length == array->capacity)
    {
        char *data = realloc(array->data, array->element_size * array->capacity * 2);
        if (data == NULL)
        {
            fprintf(stderr, "ERROR: Array allocation error.\n");
            exit(1);
        }
        array->data = data;
        array->capacity = array->capacity * 2;
    }
    memcpy(array->data + array->length * array->element_size, thing, array->element_size);
    array->length++;
}

void *Array_pop(struct Array *array)
{
    if (array->length == 0)
    {
        fprintf(stderr, "ERROR: Array is empty\n");
        exit(1);
    }
    array->length--;
    return &array->data[array->length * array->element_size];
}

void *Array_get(struct Array *array, int index)
{
    if (index >= array->length)
    {
        fprintf(stderr, "ERROR: Array index out of range. Index: %d, Range: %d\n", index, array->length - 1);
        exit(1);
    }
    return &array->data[index * array->element_size];
}

void *Array_top(struct Array *array)
{
    if (array->length == 0)
    {
        fprintf(stderr, "ERROR: Array is empty\n");
        exit(1);
    }
    return &array->data[(array->length - 1) * array->element_size];
}

#endif