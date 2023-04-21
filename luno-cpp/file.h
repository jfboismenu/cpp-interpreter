// Copyright (c) 2023 Jean-François Boismenu

#pragma once

#include <string>

namespace luno
{

class File
{
  public:
    static File read_file(const char *filename);
    const std::string filename;
    std::string content;
};

} // namespace luno
