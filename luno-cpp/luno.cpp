// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/file.h>
#include <luno-cpp/line.h>

#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{

using namespace luno;

class TranslationUnit
{
  public:
    std::vector<Line> lines;
};

enum class PreprocessorState
{
    empty_state,
    error,
    identifier,
    decimal_constant,
    directive,
    last_state = directive
};

enum class CharacterType
{
    unsupported_character,
    whitespace,
    letter,
    number,
    other,
};

enum class TokenType
{
    header_name,
    identifier,
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
    Token(TokenType type, char first_char, int line, int column)
        : _value(first_char, 1), _type(type), _line(line), _column(column)
    {
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
    int _line;
    int _column;
    TokenType _type;
};

class Parser
{
  public:
    Parser(TranslationUnit &tu) : _tu(tu)
    {
    }

    char get_next_char()
    {
        return _tu.lines[_current_line].content[_current_column];
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
    Parser parser;
    std::vector<Token> tokens;
    Token current_token;
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
    character_types[' '] = CharacterType::whitespace;
    character_types['\t'] = CharacterType::whitespace;
    character_types['\n'] = CharacterType::whitespace;

    return character_types;
}

std::array<CharacterType, 128> character_types = initialize_character_types();

PreprocessorState empty_state(ParserState &state)
{
    const char c = state.parser.get_next_char();

    if (character_types[c] == CharacterType::whitespace)
    {
        return PreprocessorState::empty_state;
    }
    else if (character_types[c] == CharacterType::letter or c == '_')
    {
        state.current_token =
            Token(TokenType::identifier, c, state.parser.current_line(), state.parser.current_column());
        return PreprocessorState::identifier;
    }
    else if (character_types[c] == CharacterType::number)
    {
        state.current_token = Token(TokenType::number, c, state.parser.current_line(), state.parser.current_column());
        if (c != '0')
        {
            return PreprocessorState::decimal_constant;
        }
        assert(c != '0');
    }
    else if (c == ';')
    {
        state.tokens.emplace_back(TokenType::punctuator, c, state.parser.current_line(), state.parser.current_column());
        return PreprocessorState::empty_state;
    }
    return PreprocessorState::error;
}

void populate_translation_unit(File &file, TranslationUnit &unit)
{
    char *start = file.content.data();
    int line_no = 0;
    for (size_t i = 0; i < file.content.size(); ++i)
    {
        if (file.content[i] == '\n')
        {
            unit.lines.emplace_back(Line{&file, std::string(start, int(&file.content[i] - start)), line_no});
            ++line_no;
            start = &file.content[i] + 1;
        }
    }
    if (&file.content.back() <= start)
    {
        unit.lines.emplace_back(Line{&file, std::string(start, int(&file.content.back() - start) + 1), line_no});
    }
}

} // namespace

int main(int, char **argv)
{
    std::cout << argv[1] << std::endl;
    File file(File::read_file(argv[1]));
    std::cout << file.content << std::endl;
    TranslationUnit tu;
    populate_translation_unit(file, tu);
    assert(tu.lines[2].content == "int i = 0;");
    assert(tu.lines[3].content == R"delim(const char *j = "this is a \n \" string";)delim");
    assert(tu.lines[4].content == "int k = 0x1234;");
    assert(tu.lines[5].content == "char l = 'c';");
    assert(tu.lines[6].content == "bool m = true;");
    assert(tu.lines[7].content == "");
    return 0;
}
