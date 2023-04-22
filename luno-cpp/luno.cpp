// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/file.h>
#include <luno-cpp/line.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

std::string _repr(const std::string &value)
{
    std::ostringstream os;
    os << '"' << value << '"';
    return os.str();
}

template <typename T> const T &_repr(const T &value)
{
    return value;
}

template <typename T> std::ostream &operator<<(std::ostream &os, const std::vector<T> &items)
{
    if (items.empty())
    {
        return os;
    }
    for (auto it = items.begin(); it != items.end() - 1; ++it)
    {
        os << _repr(*it) << ", ";
    }
    os << _repr(items.back());
    return os;
}

#define luno_assert(lfs, comp, rhs)                                                                                    \
    {                                                                                                                  \
        if (!((lfs)comp(rhs)))                                                                                         \
        {                                                                                                              \
            std::ostringstream os;                                                                                     \
            const std::string comparator(#comp);                                                                       \
            os << (lfs) << std::endl << comparator << std::endl << (rhs) << std::endl << " failed!";                   \
            throw std::runtime_error(os.str());                                                                        \
        }                                                                                                              \
    }

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
    hexadecimal_constant,
    string_literal,
    character_literal,
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
        return PreprocessorState::decimal_constant;
    }
    else if (c == '/' and state.parser.get_current_char() == '/')
    {
        state.current_token = Token(TokenType::comment, c, current_line, current_column);
        return PreprocessorState::single_line_comment;
    }
    else if (c == '"')
    {
        state.current_token = Token(TokenType::string_literal, c, current_line, current_column);
        return PreprocessorState::string_literal;
    }
    else if (c == '\'')
    {
        state.current_token = Token(TokenType::character_constant, c, current_line, current_column);
        return PreprocessorState::character_literal;
    }
    else if (character_types[c] == CharacterType::punctuator)
    {
        state.current_token = Token(TokenType::punctuator, c, current_line, current_column);
        state.flush_token();
        return PreprocessorState::empty_state;
    }

    std::cout << c << std::endl;
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

PreprocessorState decimal_state(ParserState &state)
{
    const char c = state.parser.get_current_char();

    if (character_types[c] == CharacterType::number || c == '.' || c == 'x' || c == 'X' || ('0' <= c && c <= '9') ||
        ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f'))
    {
        // We have a number, so we can append the char and move to the next
        // one while we remain in the same state. This can yield an invalid token
        // but we're not going to care for that. Once we validate our tokens
        // during the next pass we'll see right away that is was invalid.
        state.parser.advance();
        state.current_token.append(c);

        // FIXME: When parsing an hexadecimal number, we can have + and -. We do
        // not support these at the moment.
        return PreprocessorState::decimal_constant;
    }

    // We found a character that is not part of the number, so we're done.
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

PreprocessorState _string_or_character_state(ParserState &state, const char delimiter, PreprocessorState char_state)
{
    const char c = state.parser.get_current_char();

    // We're inside a string, so whatever the character is, we're adding it to
    // the literal.
    state.parser.advance();
    // We are closing the string only if the previous character wasn't
    // a backslash.
    if (c == delimiter && state.current_token.value().back() != '\\')
    {
        state.current_token.append(c);
        state.flush_token();
        return PreprocessorState::empty_state;
    }
    if (c == '\n')
    {
        state.flush_token();
        return PreprocessorState::empty_state;
    }
    state.current_token.append(c);
    return char_state;
}

PreprocessorState string_literal_state(ParserState &state)
{
    return _string_or_character_state(state, '"', PreprocessorState::string_literal);
}

PreprocessorState character_literal_state(ParserState &state)
{
    return _string_or_character_state(state, '\'', PreprocessorState::character_literal);
}

typedef PreprocessorState (*PreprocessorStateFunc)(ParserState &);

std::array<PreprocessorStateFunc, NB_PREPROCESSOR_STATES> initialize_preprocessor_funcs()
{
    std::array<PreprocessorStateFunc, NB_PREPROCESSOR_STATES> states;
    states[int(PreprocessorState::empty_state)] = empty_state;
    states[int(PreprocessorState::identifier)] = identifier_state;
    states[int(PreprocessorState::single_line_comment)] = single_line_comment_state;
    states[int(PreprocessorState::decimal_constant)] = decimal_state;
    states[int(PreprocessorState::string_literal)] = string_literal_state;
    states[int(PreprocessorState::character_literal)] = character_literal_state;
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
    luno_assert(tu.lines[0].content, ==, "// Copyright (c) 2023 Jean-François Boismenu\n");
    luno_assert(tu.lines[1].content, ==, "\n");
    luno_assert(tu.lines[2].content, ==, "int i = 0;\n");
    luno_assert(tu.lines[3].content, ==, R"delim(const char *j = "this is a \n \" string";)delim" + std::string("\n"));
    luno_assert(tu.lines[4].content, ==, "int k = 0x1234;\n");
    luno_assert(tu.lines[5].content, ==, "char l = 'c';\n");
    luno_assert(tu.lines[6].content, ==, "float d = 3.1416;\n");
    luno_assert(tu.lines[7].content, ==, "bool m = true;\n");
    luno_assert(tu.lines[8].content, ==, "");
    std::cout << "test_line_parsing passed!" << std::endl;
}

void test_tokenization(ParserState &state)
{
    parse_translation_unit(state);

    std::vector<std::string> result;
    std::transform(state.get_tokens().begin(), state.get_tokens().end(), std::back_inserter(result),
                   [](const Token &token) { return token.value(); });

    const std::vector<std::string> expected({"// Copyright (c) 2023 Jean-François Boismenu",
                                             "int",
                                             "i",
                                             "=",
                                             "0",
                                             ";",
                                             "const",
                                             "char",
                                             "*",
                                             "j",
                                             "=",
                                             "\"this is a \\n \\\" string\"",
                                             ";",
                                             "int",
                                             "k",
                                             "=",
                                             "0x1234",
                                             ";",
                                             "char",
                                             "l",
                                             "=",
                                             "'c'",
                                             ";",
                                             "float",
                                             "d",
                                             "=",
                                             "3.1416",
                                             ";",
                                             "bool",
                                             "m",
                                             "=",
                                             "true",
                                             ";"});

    luno_assert(result, ==, expected);

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
    try
    {
        run_tests();
    }
    catch (std::runtime_error &ex)
    {
        std::cout << ex.what() << std::endl;
        return 1;
    }
    return 0;
}
