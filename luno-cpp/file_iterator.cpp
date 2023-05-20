// Copyright (c) 2023 Jean-François Boismenu

//
// Created by Jean-Francois Boismenu on 2023-05-19.
//

#include <luno-cpp/file_iterator.h>

namespace luno
{

FileIterator::FileIterator(std::vector<Line> &&lines) : _lines(lines)
{
}

char FileIterator::get_current_char() const
{
    // If we're already at the end of the file, just return a whitespace.
    if (_current_line == _lines.size())
    {
        return '\0';
    }
    return _lines[_current_line].content[_current_column];
}

char FileIterator::peek_next_char()
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

void FileIterator::advance()
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

int FileIterator::current_line() const
{
    return _current_line;
}

bool FileIterator::is_finished() const
{
    return _current_line == _lines.size();
}

int FileIterator::current_column() const
{
    return _lines[_current_line].line_number;
}
} // namespace luno
