// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/assert.h>
#include <luno-cpp/luno.h>
#include <luno-cpp/preprocessor_states.h>
#include <luno-cpp/token.h>

namespace
{
using namespace luno;

class LexerState
{
  public:
    virtual LexerState *parse(Lexer &state) = 0;
};

class EmptyState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} empty_state;

class ErrorState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} error_state;

class IdentifierState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} identifier_state;

class DecimalState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} decimal_state;

class SingleLineCommentState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} single_line_comment_state;

class TwoCharPunctuatorState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} two_char_punctuator_state;

class ThreeCharPunctuatorState : public LexerState
{
    LexerState *parse(Lexer &state) override;
} three_char_punctuator_state;

class StringOrCharacterState : public LexerState
{
  public:
    StringOrCharacterState(char delimiter);

    LexerState *parse(Lexer &state) override;

  private:
    const char _delimiter;
};

StringOrCharacterState string_literal_state('"');
StringOrCharacterState character_literal_state('\'');

enum class CharacterType
{
    unsupported_character,
    whitespace,
    letter,
    number,
    single_char_punctuator,
    two_char_punctuator,
    three_char_punctuator,
    other,
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
    for (auto punctuator : ":=+-|&!~%^*/")
    {
        character_types[punctuator] = CharacterType::two_char_punctuator;
    }
    for (auto punctuator : "<>")
    {
        character_types[punctuator] = CharacterType::three_char_punctuator;
    }
    for (auto punctuator : "(){}[\\;?,.")
    {
        character_types[punctuator] = CharacterType::single_char_punctuator;
    }
    for (auto whitespace : " \t\n")
    {
        character_types[whitespace] = CharacterType::whitespace;
    }

    return character_types;
}

std::array<CharacterType, 128> character_types = initialize_character_types();

LexerState *EmptyState::parse(Lexer &state)
{
    const char c = state.iterator.get_current_char();
    const int current_line = state.iterator.current_line();
    const int current_column = state.iterator.current_column();
    state.iterator.advance();

    if (character_types[c] == CharacterType::whitespace)
    {
        return this;
    }
    else if (character_types[c] == CharacterType::letter or c == '_')
    {
        state.current_token = Token(TokenType::identifier, c, current_line, current_column);
        return &identifier_state;
    }
    else if (character_types[c] == CharacterType::number)
    {
        state.current_token = Token(TokenType::number, c, current_line, current_column);
        return &decimal_state;
    }
    else if (c == '/' and state.iterator.get_current_char() == '/')
    {
        state.current_token = Token(TokenType::comment, c, current_line, current_column);
        return &single_line_comment_state;
    }
    else if (c == '"')
    {
        state.current_token = Token(TokenType::string_literal, c, current_line, current_column);
        return &string_literal_state;
    }
    else if (c == '\'')
    {
        state.current_token = Token(TokenType::character_constant, c, current_line, current_column);
        return &character_literal_state;
    }
    else if (character_types[c] == CharacterType::two_char_punctuator)
    {
        state.current_token = Token(TokenType::punctuator, c, current_line, current_column);
        return &two_char_punctuator_state;
    }
    else if (character_types[c] == CharacterType::three_char_punctuator)
    {
        state.current_token = Token(TokenType::punctuator, c, current_line, current_column);
        return &three_char_punctuator_state;
    }
    else if (character_types[c] == CharacterType::single_char_punctuator)
    {
        state.current_token = Token(TokenType::punctuator, c, current_line, current_column);
        state.flush_token();
        return &empty_state;
    }
    return &error_state;
}

LexerState *IdentifierState::parse(Lexer &state)
{
    // If the next character is letter, number or underscore, we're still parsing an identifier.
    const char c = state.iterator.get_current_char();
    if (character_types[c] == CharacterType::letter || character_types[c] == CharacterType::number or c == '_')
    {
        state.current_token.append(c);
        state.iterator.advance();
        return this;
    }
    // Otherwise the identifier is over. We do not advance the parsing.
    state.flush_token();
    return &empty_state;
}

LexerState *DecimalState::parse(Lexer &state)
{
    const char c = state.iterator.get_current_char();

    if (character_types[c] == CharacterType::number || c == '.' || c == 'x' || c == 'X' || ('0' <= c && c <= '9') ||
        ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f'))
    {
        // We have a number, so we can append the char and move to the next
        // one while we remain in the same state. This can yield an invalid token
        // but we're not going to care for that. Once we validate our tokens
        // during the next pass we'll see right away that is was invalid.
        state.iterator.advance();
        state.current_token.append(c);

        // FIXME: When parsing an hexadecimal number, we can have + and -. We do
        // not support these at the moment.
        return this;
    }

    // We found a character that is not part of the number, so we're done.
    state.flush_token();

    return &empty_state;
}

LexerState *SingleLineCommentState::parse(Lexer &state)
{
    const char c = state.iterator.get_current_char();
    state.iterator.advance();

    if (c == '\n')
    {
        state.flush_token();
        return &empty_state;
    }
    else
    {
        state.current_token.append(c);
        return this;
    }
}

StringOrCharacterState::StringOrCharacterState(char delimiter) : _delimiter(delimiter)
{
}

LexerState *StringOrCharacterState::parse(Lexer &state)
{
    const char c = state.iterator.get_current_char();

    // We're inside a string, so whatever the character is, we're adding it to
    // the literal.
    state.iterator.advance();
    // We are closing the string only if the previous character wasn't
    // a backslash.
    if (c == _delimiter && state.current_token.value().back() != '\\')
    {
        state.current_token.append(c);
        state.flush_token();
        return &empty_state;
    }
    if (c == '\n')
    {
        state.flush_token();
        return &empty_state;
    }
    state.current_token.append(c);
    return this;
}

LexerState *TwoCharPunctuatorState::parse(Lexer &state)
{
    ;
    const char first_char = state.current_token.value()[0];
    const char next_char = state.iterator.get_current_char();

    switch (first_char)
    {
    case ':':
    case '=': {
        if (next_char == first_char)
        {
            state.iterator.advance();
            state.current_token.append(next_char);
        }
    }
    break;

    case '+':
    case '-':
    case '|':
    case '&': {
        if (first_char == next_char || next_char == '=')
        {
            state.iterator.advance();
            state.current_token.append(next_char);
        }
    }
    break;

    case '!':
    case '~':
    case '%':
    case '^':
    case '*':
    case '/': {
        if (next_char == '=')
        {
            state.iterator.advance();
            state.current_token.append(next_char);
        }
    }
    break;
    }

    state.flush_token();
    return &empty_state;
}

LexerState *ThreeCharPunctuatorState::parse(Lexer &state)
{
    const char first_char = state.current_token.value()[0];
    const char next_char = state.iterator.get_current_char();

    if (next_char == '=')
    {
        state.iterator.advance();
        state.current_token.append(next_char);
        state.flush_token();
        ;
        return &empty_state;
    }
    if (next_char == first_char && state.current_token.value().size() == 1)
    {
        state.iterator.advance();
        state.current_token.append(next_char);
        return &three_char_punctuator_state;
    }
    state.flush_token();
    return &empty_state;
}

LexerState *ErrorState::parse(Lexer &)
{
    throw std::runtime_error("Unexpected compiler error.");
}
} // namespace

namespace luno
{

void parse_translation_unit(Lexer &state)
{

    LexerState *current = &empty_state;

    while (!state.iterator.is_finished())
    {
        current = current->parse(state);
    }
}

} // namespace luno
