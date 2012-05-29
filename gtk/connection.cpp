#include "connection.hpp"

#include <memory>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/poll.h>


Connection::Connection(const std::string &host)
{
   struct sigaction sig{};
   sig.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sig, nullptr);

   struct addrinfo hints{}, *servinfo{nullptr};
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(host.c_str(), stringify(default_port).c_str(), &hints, &servinfo) < 0)
      throw std::runtime_error("getaddrinfo() failed.\n");

   std::unique_ptr<struct addrinfo, std::function<void (struct addrinfo*)>> holder(servinfo,
         [](struct addrinfo *info) {
            freeaddrinfo(info);
         });

   fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
   if (fd < 0)
      throw std::runtime_error("Failed to create socket.\n");

   if (connect(fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
   {
      close(fd);
      throw std::runtime_error("Failed to connect.\n");
   }
}

Connection::~Connection()
{
   close(fd);
}

std::string Connection::command(const std::string &cmd)
{
   if (write(fd, cmd.data(), cmd.size()) < static_cast<ssize_t>(cmd.size()))
      return "ERROR";

   struct pollfd fds{};
   fds.events = POLLIN;
   fds.fd = fd;

   if (poll(&fds, 1, 1000) < 0)
      return "ERROR";

   if (!(fds.revents & POLLIN))
      return "TIMEOUT";

   char buf[1024];
   ssize_t ret = read(fd, buf, sizeof(buf));

   if (ret <= 0)
      return "ERROR";

   return {buf, buf + ret};
}

