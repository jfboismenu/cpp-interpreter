// Copyright (c) 2023 Jean-François Boismenu

#pragma once

#include <iosfwd>
#include <vector>

namespace luno
{
std::string _repr(const std::string &value);

template <typename T> const T &_repr(const T &value)
{
    return value;
}
} // namespace luno

template <typename T> std::ostream &operator<<(std::ostream &os, const std::vector<T> &items)
{
    if (items.empty())
    {
        return os;
    }
    for (auto it = items.begin(); it != items.end() - 1; ++it)
    {
        os << luno::_repr(*it) << ", ";
    }
    os << luno::_repr(items.back());
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
