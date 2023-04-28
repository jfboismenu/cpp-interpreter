// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/line.h>
#include <luno-cpp/preprocessor_states.h>
#include <luno-cpp/token.h>

#include <array>
#include <iostream>
#include <vector>

namespace luno
{

class FileIterator
{
  public:
    FileIterator(std::vector<Line> &&lines) : _lines(lines)
    {
    }

    char get_current_char()
    {
        // If we're already at the end of the file, just return a whitespace.
        if (_current_line == _lines.size())
        {
            return '\0';
        }
        return _lines[_current_line].content[_current_column];
    }

    char peek_next_char()
    {
        // If we're already at the end of the file, just return a whitespace.
        if (_current_line == _lines.size())
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
        if (_current_line == _lines.size())
        {
            return;
        }
        // Move forward
        ++_current_column;

        // If we've hit the end of the line, move on to the next line.
        if (_lines[_current_line].content.size() == _current_column)
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
        return _lines[_current_line].line_number;
    }

    bool is_finished() const
    {
        return _current_line == _lines.size();
    }

  private:
    const std::vector<Line> _lines;
    int _current_line = 0;
    int _current_column = 0;
};

class Lexer
{
  public:
    Lexer(std::vector<Line> &&lines) : iterator(std::move(lines))
    {
    }

    FileIterator iterator;
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

} // namespace luno
