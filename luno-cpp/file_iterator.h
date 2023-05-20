// Copyright (c) 2023 Jean-François Boismenu

//
// Created by Jean-Francois Boismenu on 2023-05-19.
//

#pragma once

#include <luno-cpp/line.h>

#include <vector>

namespace luno
{

class FileIterator
{
  public:
    FileIterator(std::vector<Line> &&lines);

    char get_current_char() const;

    char peek_next_char();

    void advance();

    int current_line() const;

    int current_column() const;

    bool is_finished() const;

  private:
    const std::vector<Line> _lines;
    int _current_line = 0;
    int _current_column = 0;
};

} // namespace luno
