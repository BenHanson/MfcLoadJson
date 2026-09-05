#pragma once

enum json_type
{
    Array = 1,
    Boolean = 1 << 1,
    Document = 1 << 2,
    Double = 1 << 3,
    Int64 = 1 << 4,
    Key = 1 << 5,
    Null = 1 << 6,
    Object = 1 << 7,
    String = 1 << 8,
    UInt64 = 1 << 9
};
