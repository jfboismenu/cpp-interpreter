// Copyright (c) 2023 Jean-François Boismenu

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace luno
{
namespace line_parser
{

class File
{
  public:
    const std::string filename;
    std::string content;
};

class Line
{
  public:
    const File *const file;
    char *start;
    const int length;
    const int line_number;
    std::string content() const
    {
        return std::string(start, length);
    }
};

} // namespace line_parser

} // namespace luno

using namespace luno::line_parser;

File read_file(const char *filename)
{
    std::ifstream file(filename);
    assert(file.is_open());
    const int length(file.seekg(0, std::ios_base::end).tellg());
    file.seekg(0, std::ios_base::beg);
    std::string content(length, '\0');
    file.read(content.data(), length);
    return File{std::string(filename), content};
}

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
};

void populate_translation_unit(File &file, TranslationUnit &unit)
{
    char *start = file.content.data();
    int line_no = 0;
    for (size_t i = 0; i < file.content.size(); ++i)
    {
        if (file.content[i] == '\n')
        {
            unit.lines.emplace_back(Line{&file, start, int(&file.content[i] - start), line_no});
            ++line_no;
            start = &file.content[i] + 1;
        }
    }
    if (&file.content.back() <= start)
    {
        unit.lines.emplace_back(Line{&file, start, int(&file.content.back() - start) + 1, line_no});
    }
}

int main(int, char **argv)
{
    std::cout << argv[1] << std::endl;
    File file(read_file(argv[1]));
    std::cout << file.content << std::endl;
    TranslationUnit tu;
    populate_translation_unit(file, tu);
    assert(tu.lines[2].content() == "int i = 0;");
    assert(tu.lines[3].content() == R"delim(const char *j = "this is a \n \" string";)delim");
    assert(tu.lines[4].content() == "int k = 0x1234;");
    assert(tu.lines[5].content() == "char l = 'c';");
    assert(tu.lines[6].content() == "bool m = true;");
    assert(tu.lines[7].content() == "");
    return 0;
}
