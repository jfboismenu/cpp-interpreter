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

// class Token {
//     const char *const start;
//     const char *const end;
// };

enum class PreprocessorState
{
    start_of_line,
    identifier,
    decimal,
    octal,
    binary,
    floating,
    hexadecimal,
    directive,
    last_state = directive
};

enum class StateTransition
{
    identifier_char,
    whitespace,
    number,
};

std::array<StateTransition, 128> character_types;

void populate_character_types()
{
    for (auto letter = 'a'; letter <= 'z'; ++letter)
    {
        character_types[letter] = StateTransition::identifier_char;
    }
    for (auto letter = 'A'; letter <= 'Z'; ++letter)
    {
        character_types[letter] = StateTransition::identifier_char;
    }
    for (auto letter = '0'; letter <= '9'; ++letter)
    {
        character_types[letter] = StateTransition::number;
    }
    character_types['_'] = StateTransition::identifier_char;
    character_types[' '] = StateTransition::whitespace;
    character_types['\t'] = StateTransition::whitespace;
    character_types['\n'] = StateTransition::whitespace;
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
