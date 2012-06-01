#ifndef CONNECTION_HPP__
#define CONNECTION_HPP__

#include "../utils.hpp"
#include <string>
#include <stdexcept>
#include <cstddef>

class Connection
{
   public:
      Connection(const std::string &host = "localhost");
      ~Connection();
      std::string command(const std::string &cmd);

   private:
      enum { default_port = 42878 };
      int fd;

      void write_all(const char *data, std::size_t size);
};

#endif

