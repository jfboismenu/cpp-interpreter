// Copyright (c) 2023 Jean-François Boismenu

//
// Created by Jean-Francois Boismenu on 2023-05-19.
//

#include <luno-cpp/lexer.h>

namespace luno
{
Lexer::Lexer(std::__1::vector<luno::Line> &&lines) : iterator(std::move(lines))
{
}

void Lexer::flush_token()
{
    _tokens.emplace_back(current_token);
    current_token = luno::Token();
}

const std::vector<luno::Token> &Lexer::get_tokens() const
{
    return _tokens;
}
} // namespace luno
