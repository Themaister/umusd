#include "tcpcommand.hpp"
#include "utils.hpp"
#include "player.hpp"

#include <memory>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>

TCPCommand::TCPCommand(std::uint16_t port) : remote(nullptr)
{
   struct addrinfo hints{}, *servinfo{nullptr};

   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_PASSIVE;

   if (getaddrinfo(nullptr, stringify(port).c_str(), &hints, &servinfo) < 0)
      throw std::runtime_error("getaddrinfo() failed.\n");

   std::unique_ptr<struct addrinfo, std::function<void (struct addrinfo*)>> holder(servinfo,
         [](struct addrinfo *info) {
            freeaddrinfo(info);
         });

   fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
   if (fd < 0)
      throw std::runtime_error("Failed to create socket.\n");

   int yes = 1;
   setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
   if (bind(fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
   {
      close(fd);
      throw std::runtime_error("Failed to bind socket. umusd is probably already running.\n");
   }

   if (listen(fd, 10) < 0)
   {
      close(fd);
      throw std::runtime_error("Failed to listen to socket.\n");
   }
}

void TCPCommand::handle(EventHandler &handler)
{
   int newfd = accept(fd, nullptr, nullptr);
   if (newfd < 0)
      return;
   
   connections.erase(std::remove_if(std::begin(connections), std::end(connections),
         [](std::shared_ptr<TCPSocket> sock) { return sock->dead(); }),
         connections.end());

   auto conn = std::make_shared<TCPSocket>(newfd);
   handler.add(conn);
   conn->set_remote(*remote);
   connections.push_back(conn);
}

EventHandled::PollList TCPCommand::pollfds() const
{
   return {{fd, EPOLLIN}};
}

void TCPCommand::set_remote(Remote &remote)
{
   this->remote = &remote;
   for (auto &sock : connections)
      sock->set_remote(remote);
}

TCPCommand& TCPCommand::operator=(TCPCommand &&tcp)
{
   connections = std::move(tcp.connections);

   if (fd >= 0)
      close(fd);
   fd = -1;
   std::swap(fd, tcp.fd);

   remote = tcp.remote;
   return *this;
}

TCPCommand::TCPCommand(TCPCommand &&tcp)
{
   *this = std::move(tcp);
}

TCPSocket::TCPSocket(int fd) : fd(fd), is_dead(false)
{
   struct sigaction sig{};
   sig.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sig, nullptr);
}

TCPSocket::~TCPSocket() { kill_sock(); }

EventHandled::PollList TCPSocket::pollfds() const
{
   return {{fd, EPOLLIN}};
}

void TCPSocket::kill_sock()
{
   if (fd >= 0)
      close(fd);
   fd = -1;
   is_dead = true;
}

void TCPSocket::write_all(const char *data, std::size_t size)
{
   while (size)
   {
      ssize_t ret = write(fd, data, size);
      if (ret <= 0)
         throw std::runtime_error("Failed writing to command socket.");

      data += ret;
      size -= ret;
   }
}

void TCPSocket::parse_commands(EventHandler &event)
{
   for (;;)
   {
      auto first = command_buf.find("\r\n");
      if (first == std::string::npos)
         return;

      auto cmd = command_buf.substr(0, first);

      auto reply = parse_command(event, cmd);
      reply += "\r\n";
      write_all(reply.data(), reply.size());

      command_buf.erase(0, first + 2);
   }
}

void TCPSocket::handle(EventHandler &event)
{
   flush_buffer();

   try
   {
      parse_commands(event);
   }
   catch (const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
      kill_sock();
   }
}

void TCPSocket::flush_buffer()
{
   char buf[1024];
   ssize_t ret = ::read(fd, buf, sizeof(buf));
   if (ret <= 0)
   {
      kill_sock();
      return;
   }

   command_buf.insert(command_buf.end(), buf, buf + ret);
}

bool TCPSocket::dead() const
{
   return is_dead;
}

TCPSocket::TCPSocket(TCPSocket &&tcp)
{
   *this = std::move(tcp);
}

TCPSocket& TCPSocket::operator=(TCPSocket &&tcp)
{
   kill_sock();

   std::swap(tcp.fd, fd);
   std::swap(tcp.is_dead, is_dead);
   remote = tcp.remote;

   return *this;
}

