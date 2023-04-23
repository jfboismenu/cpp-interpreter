// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/token.h>

namespace luno
{

Token::Token(TokenType type, char first_char, int line, int column)
    : _value(1, first_char), _type(type), _line(line), _column(column)
{
}

void Token::append(char c)
{
    _value.push_back(c);
}

TokenType Token::type() const
{
    return _type;
}

int Token::line() const
{
    return _line;
}

int Token::column() const
{
    return _column;
}

std::string Token::value() const
{
    return _value;
}

} // namespace luno
