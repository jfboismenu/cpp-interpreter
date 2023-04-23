// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/assert.h>
#include <luno-cpp/file.h>
#include <luno-cpp/line.h>
#include <luno-cpp/luno.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace
{

using namespace luno;

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

void test_tokenization(Lexer &state)
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

    Lexer state{Lexer(tu)};
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
