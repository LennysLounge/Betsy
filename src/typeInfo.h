#pragma once

#include <assert.h>

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
