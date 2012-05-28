#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <sstream>
#include <string>
#include <vector>

template <class T>
std::string stringify(T&& t)
{
   std::ostringstream stream;
   stream << std::forward<T>(t);
   return stream.str();
}

template <class T, class... U>
std::string stringify(T&& t, U&&... u)
{
   std::ostringstream stream;
   stream << std::forward<T>(t) << stringify(u...);
   return stream.str();
}

#endif

