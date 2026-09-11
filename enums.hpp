#pragma once

enum json_type
{
    Array = 1,
    Boolean = 1 << 1,
    Document = 1 << 2,
    Key = 1 << 3,
    Null = 1 << 4,
    Object = 1 << 5,
    Number = 1 << 6,
    String = 1 << 7,
    Scalar = Boolean | Number | String
};

enum
{
    WM_POPULATE_DATA = WM_USER + 1,
    WM_FINISHED_EDITING
};
