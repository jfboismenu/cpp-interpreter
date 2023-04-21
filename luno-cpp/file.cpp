// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/file.h>

#include <fstream>
#include <string>

namespace luno
{

File File::read_file(const char *filename)
{
    std::ifstream file(filename);
    assert(file.is_open());
    const int length(file.seekg(0, std::ios_base::end).tellg());
    file.seekg(0, std::ios_base::beg);
    std::string content(length, '\0');
    file.read(content.data(), length);
    return File{std::string(filename), content};
}

} // namespace luno
