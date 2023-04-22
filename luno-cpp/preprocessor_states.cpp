// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/luno.h>
#include <luno-cpp/preprocessor_states.h>

namespace
{
using namespace luno;

class ParserState;
class PreprocessorState
{
  public:
    virtual PreprocessorState *parse(ParserState &state) = 0;
};

class EmptyState : public PreprocessorState
{
    PreprocessorState *parse(ParserState &state) override;
} empty_state;

class ErrorState : public PreprocessorState
{
    PreprocessorState *parse(ParserState &state) override;
} error_state;

class IdentifierState : public PreprocessorState
{
    PreprocessorState *parse(ParserState &state) override;
} identifier_state;

class DecimalState : public PreprocessorState
{
    PreprocessorState *parse(ParserState &state) override;
} decimal_state;

class SingleLineCommentState : public PreprocessorState
{
    PreprocessorState *parse(ParserState &state) override;
} single_line_comment_state;

class StringOrCharacterState : public PreprocessorState
{
  public:
    StringOrCharacterState(char delimiter);

    PreprocessorState *parse(ParserState &state);

  private:
    const char _delimiter;
};

StringOrCharacterState string_literal_state('"');
StringOrCharacterState character_literal_state('\'');

PreprocessorState *EmptyState::parse(ParserState &state){{const char c = state.parser.get_current_char();
const int current_line = state.parser.current_line();
const int current_column = state.parser.current_column();
state.parser.advance();

if (character_types[c] == CharacterType::whitespace)
{
    return this;
}
else if (character_types[c] == CharacterType::letter or c == '_')
{
    state.current_token = Token(TokenType::identifier, c, current_line, current_column);
    return &identifier;
}
else if (character_types[c] == CharacterType::number)
{
    state.current_token = Token(TokenType::number, c, current_line, current_column);
    return &decimal_constant;
}
else if (c == '/' and state.parser.get_current_char() == '/')
{
    state.current_token = Token(TokenType::comment, c, current_line, current_column);
    return &single_line_comment;
}
else if (c == '"')
{
    state.current_token = Token(TokenType::string_literal, c, current_line, current_column);
    return &string_literal;
}
else if (c == '\'')
{
    state.current_token = Token(TokenType::character_constant, c, current_line, current_column);
    return &character_literal;
}
else if (character_types[c] == CharacterType::punctuator)
{
    state.current_token = Token(TokenType::punctuator, c, current_line, current_column);
    state.flush_token();
    return &empty_state;
}

std::cout << c << std::endl;
return &error_state;
} // namespace
}
;

PreprocessorState *IdentifierState::parse(ParserState &state)
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

PreprocessorState *DecimalState::parse(ParserState &state)
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

PreprocessorState *SingleLineCommentState::parse(ParserState &state)
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

StringOrCharacterState::StringOrCharacterState(char delimiter) : _delimiter(delimiter)
{
}

PreprocessorState *StringOrCharacterState::parse(ParserState &state)
{
    const char c = state.parser.get_current_char();

    // We're inside a string, so whatever the character is, we're adding it to
    // the literal.
    state.parser.advance();
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

PreprocessorState *ErrorState::parse(ParserState &)
{
    throw std::runtime_error("Unexpected compiler error.");
}
}

namespace luno
{

void parse_translation_unit(ParserState &state);
{

    PreprocessorState *current = &initial_state;

    while (!state.parser.is_finished())
    {
        current = current->parse(state);
    }
}
} // namespace luno