// Copyright (c) 2023 Jean-François Boismenu

#include <luno-cpp/assert.h>
#include <sstream>

namespace luno
{
std::string _repr(const std::string &value)
{
    std::ostringstream os;
    os << '"' << value << '"';
    return os.str();
}
} // namespace luno
