
// Copyright (c) 2023 Jean-François Boismenu

#pragma once
#include <string>

namespace luno
{

enum class TokenType
{
    invalid,
    identifier,
    comment,
    number,
    character_constant,
    string_literal,
    operator_,
    punctuator,
    preprocessor_directive
};

class Token
{
  public:
    Token() = default;
    Token &operator=(const Token &) = default;
    Token(TokenType type, char first_char, int line, int column);

    void append(char c);

    TokenType type() const;

    int line() const;

    int column() const;

    std::string value() const;

  private:
    std::string _value;
    int _line = 0;
    int _column = 0;
    TokenType _type = TokenType::invalid;
};

} // namespace luno
