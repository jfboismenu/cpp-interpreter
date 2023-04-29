// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/assert.h>
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

std::string read_file(const std::filesystem::path &path)
{
    std::ifstream file(path.c_str());
    assert(file.is_open());

    const size_t size = file.seekg(0, std::ios_base::end).tellg();
    file.seekg(0, std::ios_base::beg);
    std::string content(size, 0);
    file.read(&content[0], size);

    return content;
}

std::vector<Line> split_text_into_lines(std::string &&text)
{
    std::vector<Line> lines;
    int line_number = 0;

    char *start = &text.front();
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            std::string line(start, int(&text[i] - start + 1));
            lines.emplace_back(Line{line, line_number});
            ++line_number;
            start = &text[i] + 1;
        }
    }

    if (start < &text.back())
    {
        lines.emplace_back(Line{std::string(start, int(&text.back() - start)), line_number});
    }
    return lines;
}

void test_line_parsing()
{
    const auto current_file = std::filesystem::path(__FILE__);
    const auto parent_folder = current_file.parent_path();
    const auto test_file = parent_folder / "test.cpp";

    std::string text = read_file(test_file);
    std::vector<Line> lines = split_text_into_lines(std::move(text));

    luno_assert(lines[0].content, ==, "// Copyright (c) 2023 Jean-François Boismenu\n");
    luno_assert(lines[1].content, ==, "\n");
    luno_assert(lines[2].content, ==, "int i = 0;\n");
    luno_assert(lines[3].content, ==, R"delim(const char *j = "this is a \n \" string";)delim" + std::string("\n"));
    luno_assert(lines[4].content, ==, "int k = 0x1234;\n");
    luno_assert(lines[5].content, ==, "char l = 'c';\n");
    luno_assert(lines[6].content, ==, "float d = 3.1416;\n");
    luno_assert(lines[7].content, ==, "bool m = true;\n");
    luno_assert(lines[8].content, ==, "");
    std::cout << "test_line_parsing passed!" << std::endl;
}

std::vector<std::string> _tokens_to_string(const std::vector<Token> &tokens)
{
    std::vector<std::string> result;
    std::transform(tokens.begin(), tokens.end(), std::back_inserter(result),
                   [](const Token &token) { return token.value(); });

    return result;
}

void _test_tokenization(const std::string &code, const std::vector<std::string> &expected)
{
    Lexer lexer({Line{code, 0}});
    parse_translation_unit(lexer);
    luno_assert(_tokens_to_string(lexer.get_tokens()), ==, expected);
}

void test_tokenization()
{
    _test_tokenization("// Copyright (c) 2023 Jean-François Boismenu",
                       {"// Copyright (c) 2023 Jean-François Boismenu"});
    _test_tokenization("int/*hello*/ world /* this is a multi*\nline comment*/",
                       {"int", "/*hello*/", "world", "/* this is a multi*line comment*/"});

    _test_tokenization("int i = 0;", {"int", "i", "=", "0", ";"});

    _test_tokenization("const char *j = \"this is a \\n \\\" string\";",
                       {"const", "char", "*", "j", "=", "\"this is a \\n \\\" string\"", ";"});

    _test_tokenization("int k = 0x1234;", {"int", "k", "=", "0x1234", ";"});
    _test_tokenization("char l = 'c';", {"char", "l", "=", "'c'", ";"});
    _test_tokenization("float d = 3.1416;", {"float", "d", "=", "3.1416", ";"});
    _test_tokenization("bool m = true;", {"bool", "m", "=", "true", ";"});

    _test_tokenization("((a))", {"(", "(", "a", ")", ")"});
    _test_tokenization("[[a]]", {"[", "[", "a", "]", "]"});
    _test_tokenization(",,,", {",", ",", ","});
    _test_tokenization("???", {"?", "?", "?"});
    _test_tokenization("a.b", {"a", ".", "b"});
    _test_tokenization("+++++", {"++", "++", "+"});
    _test_tokenization(":::::", {"::", "::", ":"});
    _test_tokenization("-----", {"--", "--", "-"});
    _test_tokenization("&&&&&", {"&&", "&&", "&"});
    _test_tokenization("|||||", {"||", "||", "|"});
    _test_tokenization("=====", {"==", "==", "="});
    _test_tokenization("<<<<<", {"<<", "<<", "<"});
    _test_tokenization(">>>>>", {">>", ">>", ">"});
    _test_tokenization("<<<<=<", {"<<", "<<=", "<"});
    _test_tokenization(">>>>=>", {">>", ">>=", ">"});
    _test_tokenization("a|=b", {"a", "|=", "b"});
    _test_tokenization("a+=b", {"a", "+=", "b"});
    _test_tokenization("a-=b", {"a", "-=", "b"});
    _test_tokenization("a*=b", {"a", "*=", "b"});
    _test_tokenization("a/=b", {"a", "/=", "b"});
    _test_tokenization("a!=b", {"a", "!=", "b"});
    _test_tokenization("a~=b", {"a", "~=", "b"});
    _test_tokenization("a&=b", {"a", "&=", "b"});
    _test_tokenization("a%=b", {"a", "%=", "b"});
    _test_tokenization("a^=b", {"a", "^=", "b"});
    _test_tokenization("a:b", {"a", ":", "b"});
    _test_tokenization("a=b", {"a", "=", "b"});
    _test_tokenization("a+b", {"a", "+", "b"});
    _test_tokenization("a-b", {"a", "-", "b"});
    _test_tokenization("a|b", {"a", "|", "b"});
    _test_tokenization("a&b", {"a", "&", "b"});
    _test_tokenization("a!b", {"a", "!", "b"});
    _test_tokenization("a~b", {"a", "~", "b"});
    _test_tokenization("a%b", {"a", "%", "b"});
    _test_tokenization("a^b", {"a", "^", "b"});
    _test_tokenization("a*b", {"a", "*", "b"});
    _test_tokenization("a/b", {"a", "/", "b"});
    _test_tokenization("a/b", {"a", "/", "b"});
    _test_tokenization("a->b", {"a", "->", "b"});
    _test_tokenization("a.*b", {"a", ".*", "b"});
    _test_tokenization("a->*b", {"a", "->*", "b"});

    std::cout << "test_tokenization passed!" << std::endl;
}

void run_tests()
{
    test_line_parsing();

    test_tokenization();
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
