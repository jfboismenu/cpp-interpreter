// Copyright (c) 2023 Jean-François Boismenu

//
// Created by Jean-Francois Boismenu on 2023-05-19.
//

#pragma once

#include <luno-cpp/file_iterator.h>
#include <luno-cpp/token.h>
#include <vector>

namespace luno
{
class Lexer
{
  public:
    Lexer(std::__1::vector<luno::Line> &&lines);

    luno::FileIterator iterator;
    luno::Token current_token;

    void flush_token();

    const std::vector<luno::Token> &get_tokens() const;

  private:
    std::vector<luno::Token> _tokens;
};
} // namespace luno
