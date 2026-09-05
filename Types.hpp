#pragma once

#include "enums.hpp"

struct item
{
    HTREEITEM _tree_item;
    json_type _type;

    item(const HTREEITEM tree_item, const json_type type) :
        _tree_item(tree_item),
        _type(type)
    {}
};

enum class whitespace { no, yes };
