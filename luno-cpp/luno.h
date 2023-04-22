// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/file.h>
#include <luno-cpp/line.h>
#include <luno-cpp/preprocessor_states.h>

#include <array>
#include <iostream>
#include <vector>

namespace luno
{
class TranslationUnit
{
  public:
    std::vector<Line> lines;
};

enum class CharacterType
{
    unsupported_character,
    whitespace,
    letter,
    number,
    punctuator,
    other,
};

enum class TokenType
{
    invalid,
    header_name,
    identifier,
    comment,
    number,
    character_constant,
    string_literal,
    operator_,
    punctuator,
    other
};

class Token
{
  public:
    Token() = default;
    Token &operator=(const Token &) = default;
    Token(TokenType type, char first_char, int line, int column)
        : _value(1, first_char), _type(type), _line(line), _column(column)
    {
    }

    void append(char c)
    {
        _value.push_back(c);
    }

    TokenType type() const
    {
        return _type;
    }

    int line() const
    {
        return _line;
    }

    int column() const
    {
        return _column;
    }

    std::string value() const
    {
        return _value;
    }

  private:
    std::string _value;
    int _line = 0;
    int _column = 0;
    TokenType _type = TokenType::invalid;
};

class Parser
{
  public:
    Parser(TranslationUnit &tu) : _tu(tu)
    {
    }

    char get_current_char()
    {
        // If we're already at the end of the file, just return a whitespace.
        if (_current_line == _tu.lines.size())
        {
            return '\0';
        }
        return _tu.lines[_current_line].content[_current_column];
    }

    char peek_next_char()
    {
        // If we're already at the end of the file, just return a whitespace.
        if (_current_line == _tu.lines.size())
        {
            return '\0';
        }
        // backup the current position
        const int current_column(_current_column);
        const int current_line(_current_line);
        // Advance and get the next char
        advance();
        const char c = get_current_char();
        // Restore the current position.
        _current_column = current_column;
        _current_line = current_line;

        return c;
    }

    void advance()
    {
        if (_current_line == _tu.lines.size())
        {
            return;
        }
        // Move forward
        ++_current_column;

        // If we've hit the end of the line, move on to the next line.
        if (_tu.lines[_current_line].content.size() == _current_column)
        {
            _current_column = 0;
            _current_line += 1;
        }
    }

    int current_line() const
    {
        return _current_line;
    }

    int current_column() const
    {
        return _current_column;
    }

    bool is_finished() const
    {
        return _current_line == _tu.lines.size();
    }

  private:
    TranslationUnit &_tu;
    int _current_line = 0;
    int _current_column = 0;
};

class ParserState
{
  public:
    ParserState(TranslationUnit &tu) : parser(tu)
    {
    }

    Parser parser;
    Token current_token;

    void flush_token()
    {
        _tokens.emplace_back(current_token);
        std::cout << current_token.value() << std::endl;
        current_token = Token();
    }

    const std::vector<Token> &get_tokens() const
    {
        return _tokens;
    }

  private:
    std::vector<Token> _tokens;
};

std::array<CharacterType, 128> initialize_character_types()
{
    std::array<CharacterType, 128> character_types;
    for (auto &type : character_types)
    {
        type = CharacterType::unsupported_character;
    }
    for (auto letter = 'a'; letter <= 'z'; ++letter)
    {
        character_types[letter] = CharacterType::letter;
    }
    for (auto letter = 'A'; letter <= 'Z'; ++letter)
    {
        character_types[letter] = CharacterType::letter;
    }
    for (auto number = '0'; number <= '9'; ++number)
    {
        character_types[number] = CharacterType::number;
    }
    for (auto punctuator : "!%^&*()-+={}|~[\\;':\"<>?,./#)")
    {
        character_types[punctuator] = CharacterType::punctuator;
    }
    for (auto whitespace : " \t\n")
    {
        character_types[whitespace] = CharacterType::whitespace;
    }

    return character_types;
}

std::array<CharacterType, 128> character_types = initialize_character_types();

void populate_translation_unit(File &file, TranslationUnit &unit)
{
    char *start = file.content.data();
    int line_no = 0;
    for (size_t i = 0; i < file.content.size(); ++i)
    {
        if (file.content[i] == '\n')
        {
            std::string content(start, int(&file.content[i] - start + 1));
            unit.lines.emplace_back(Line{&file, content, line_no});
            ++line_no;
            start = &file.content[i] + 1;
        }
    }
    if (start < &file.content.back())
    {
        unit.lines.emplace_back(Line{&file, std::string(start, int(&file.content.back() - start)), line_no});
    }
}
