// Copyright (c) 2023 Jean-François Boismenu

#pragma once

#include <string>

namespace luno
{

class File;

class Line
{
  public:
    const File *const file;
    const std::string content;
    const int line_number;
};

} // namespace luno
