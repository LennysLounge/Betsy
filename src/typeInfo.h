#pragma once

#include <assert.h>
#include <string.h>

enum Type_info
{
    TYPE_INFO_INT,
    TYPE_INFO_BOOL,
    TYPE_INFO_COUNT
};

char *Type_info_name(enum Type_info type)
{
    _Static_assert(TYPE_INFO_COUNT == 2, "Exhaustive handling of all types.");
    switch (type)
    {
    case TYPE_INFO_INT:
        return "int";
    case TYPE_INFO_BOOL:
        return "bool";
    default:
        assert(0 && "unknown type in Type_info_name");
        return "";
    }
}

enum Type_info Type_info_by_name(char *word)
{
    _Static_assert(TYPE_INFO_COUNT == 2, "Exhaustive handling of all types.");
    if (strcmp(word, "int") == 0)
        return TYPE_INFO_INT;
    else if (strcmp(word, "bool") == 0)
        return TYPE_INFO_BOOL;
    else
        return -1;
}
