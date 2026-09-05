#pragma once

#include "Types.hpp"

#include <string>

std::string Export(const HTREEITEM hItem, const CTreeCtrl& tree,
    const whitespace ws);
