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
    single_line_comment,
    multi_line_comment,
    first = empty_state,
    last = multi_line_comment
};

const int NB_PREPROCESSOR_STATES = int(PreprocessorState::last) - int(PreprocessorState::first) + 1;

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
        return _tu.lines[_current_line].content[_current_column];
    }

    char peek_next_char()
    {
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

    std::vector<Token> get_tokens() const
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

PreprocessorState empty_state(ParserState &state)
{
    const char c = state.parser.get_current_char();
    const int current_line = state.parser.current_line();
    const int current_column = state.parser.current_column();
    state.parser.advance();

    if (character_types[c] == CharacterType::whitespace)
    {
        return PreprocessorState::empty_state;
    }
    else if (character_types[c] == CharacterType::letter or c == '_')
    {
        state.current_token = Token(TokenType::identifier, c, current_line, current_column);
        return PreprocessorState::identifier;
    }
    else if (character_types[c] == CharacterType::number)
    {
        state.current_token = Token(TokenType::number, c, current_line, current_column);
        if (c != '0')
        {
            return PreprocessorState::decimal_constant;
        }
        assert(c != '0');
    }
    else if (c == '/' and state.parser.get_current_char() == '/')
    {
        state.current_token = Token(TokenType::comment, c, current_line, current_column);
        return PreprocessorState::single_line_comment;
    }
    else if (character_types[c] == CharacterType::punctuator)
    {
        state.current_token = Token(TokenType::punctuator, c, current_line, current_column);
        state.flush_token();
        return PreprocessorState::empty_state;
    }

    return PreprocessorState::error;
}

PreprocessorState identifier_state(ParserState &state)
{
    // If the next character is letter, number or underscore, we're still parsing an identifier.
    const char c = state.parser.get_current_char();
    if (character_types[c] == CharacterType::letter || character_types[c] == CharacterType::number or c == '_')
    {
        state.current_token.append(c);
        state.parser.advance();
        return PreprocessorState::identifier;
    }
    // Otherwise the identifier is over. We do not advance the parsing.
    state.flush_token();
    return PreprocessorState::empty_state;
}

PreprocessorState single_line_comment_state(ParserState &state)
{
    const char c = state.parser.get_current_char();
    state.parser.advance();

    if (c == '\n')
    {
        state.flush_token();
        return PreprocessorState::empty_state;
    }
    else
    {
        state.current_token.append(c);
        return PreprocessorState::single_line_comment;
    }
}

typedef PreprocessorState (*PreprocessorStateFunc)(ParserState &);

std::array<PreprocessorStateFunc, NB_PREPROCESSOR_STATES> initialize_preprocessor_funcs()
{
    std::array<PreprocessorStateFunc, NB_PREPROCESSOR_STATES> states;
    states[int(PreprocessorState::empty_state)] = empty_state;
    states[int(PreprocessorState::identifier)] = identifier_state;
    states[int(PreprocessorState::single_line_comment)] = single_line_comment_state;
    return states;
}

std::array<PreprocessorStateFunc, NB_PREPROCESSOR_STATES> preprocessor_funcs = initialize_preprocessor_funcs();

void populate_translation_unit(File &file, TranslationUnit &unit)
{
    char *start = file.content.data();
    int line_no = 0;
    for (size_t i = 0; i < file.content.size(); ++i)
    {
        if (file.content[i] == '\n')
        {
            unit.lines.emplace_back(Line{&file, std::string(start, int(&file.content[i] - start + 1)), line_no});
            ++line_no;
            start = &file.content[i] + 1;
        }
    }
    if (&file.content.back() <= start)
    {
        unit.lines.emplace_back(Line{&file, std::string(start, int(&file.content.back() - start + 1)), line_no});
    }
}

void parse_translation_unit(ParserState &state)
{

    PreprocessorState current = PreprocessorState::empty_state;

    while (!state.parser.is_finished())
    {
        current = preprocessor_funcs[int(current)](state);
        if (current == PreprocessorState::error)
        {
            throw std::runtime_error("Unexpected compiler error.");
        }
    }
}

void test_line_parsing(File &file, TranslationUnit &tu)
{
    populate_translation_unit(file, tu);
    assert(tu.lines[0].content == "// Copyright (c) 2023 Jean-François Boismenu\n");
    assert(tu.lines[1].content == "\n");
    assert(tu.lines[2].content == "int i = 0;\n");
    assert(tu.lines[3].content == R"delim(const char *j = "this is a \n \" string";)delim" + std::string("\n"));
    assert(tu.lines[4].content == "int k = 0x1234;\n");
    assert(tu.lines[5].content == "char l = 'c';\n");
    assert(tu.lines[6].content == "bool m = true;\n");
    assert(tu.lines[7].content == "");
    std::cout << "test_line_parsing passed!" << std::endl;
}

void test_tokenization(ParserState &state)
{
    parse_translation_unit(state);
    std::cout << "test_tokenization passed!" << std::endl;
}

void run_tests()
{

    const auto current_file = std::filesystem::path(__FILE__);
    const auto parent_folder = current_file.parent_path();
    const auto test_file = parent_folder / "test.cpp";

    File file(File::read_file(test_file.c_str()));

    TranslationUnit tu;
    test_line_parsing(file, tu);

    ParserState state{ParserState(tu)};
    test_tokenization(state);
}

} // namespace

int main(int, char **argv)
{
    run_tests();
    return 0;
}
