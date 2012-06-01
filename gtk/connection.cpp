#include "connection.hpp"

#include <memory>
#include <iostream>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/poll.h>


Connection::Connection(const std::string &host) : fd(-1)
{
   struct sigaction sig{};
   sig.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sig, nullptr);

   struct addrinfo hints{}, *servinfo{nullptr};
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(host.c_str(), stringify(default_port).c_str(), &hints, &servinfo) < 0)
      throw std::runtime_error("getaddrinfo() failed.");

   std::unique_ptr<struct addrinfo, std::function<void (struct addrinfo*)>> holder(servinfo,
         [](struct addrinfo *info) {
            freeaddrinfo(info);
         });

   auto tmp = servinfo;
   bool connected = false;
   while (tmp)
   {
      fd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
      if (fd < 0)
         throw std::runtime_error("Failed to create socket.");

      if (connect(fd, tmp->ai_addr, tmp->ai_addrlen) == 0)
      {
         connected = true;
         break;
      }

      tmp = tmp->ai_next;
      close(fd);
   }

   if (!connected)
      throw std::runtime_error("Failed to connect.");
}

Connection::~Connection()
{
   close(fd);
}

void Connection::write_all(const char *data, std::size_t size)
{
   while (size)
   {
      ssize_t ret = write(fd, data, size);
      if (ret <= 0)
         throw std::runtime_error("Failed to write to socket.");

      data += ret;
      size -= ret;
   }
}

std::string Connection::command(const std::string &cmd)
{
   write_all(cmd.data(), cmd.size());

   struct pollfd fds{};
   fds.events = POLLIN;
   fds.fd = fd;

   if (poll(&fds, 1, 1000) < 0)
      throw std::runtime_error("Failed to poll socket.");

   if (!(fds.revents & POLLIN))
      throw std::runtime_error("Poll timed out.");

   char buf[1024];
   ssize_t ret = read(fd, buf, sizeof(buf));

   if (ret <= 0)
      throw std::runtime_error("Failed to read reply.");

   std::string str{buf, buf + ret};
   str.erase(str.rfind("\r\n"));

   return str;
}

