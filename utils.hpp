#ifndef UTILS_HPP__
#define UTILS_HPP__

#include <sstream>
#include <string>
#include <vector>
#include <cstddef>
#include <string.h>

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
   stream << std::forward<T>(t) << stringify(std::forward<U>(u)...);
   return stream.str();
}

inline std::vector<std::string> string_split(const std::string &str, const std::string &delim)
{
   std::vector<std::string> res;
   auto tmp = str;

   char *cookie;
   const char *s = strtok_r(&tmp[0], delim.c_str(), &cookie);
   while (s)
   {
      res.push_back(s);
      s = strtok_r(nullptr, delim.c_str(), &cookie);
   }

   return res;
}

inline std::string string_join(const std::vector<std::string> &list, const std::string &delim)
{
   std::string res;
   for (auto &str : list)
   {
      res += str;
      res += delim;
   }

   return res;
}

#endif

