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
      throw std::runtime_error("Failed to bind socket.\n");
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


TCPSocket::TCPSocket(int fd) : fd(fd), remote(nullptr), is_dead(false)
{
   init_command_map();

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

template <class Delegate>
inline std::string plain_action(Delegate func)
{
   try
   {
      func();
      return "OK";
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
      return "ERROR";
   }
}

template <class Delegate>
inline std::string metadata_action(const Remote &remote, Delegate func)
{
   try
   {
      return func(remote.media_info());
   }
   catch(const std::exception &e)
   {
      std::cerr << e.what() << std::endl;
      return "";
   }
}

void TCPSocket::init_command_map()
{
   command_map["NOOP"] = [](EventHandler &, const std::string &arg) -> std::string {
      std::cerr << "NOOP (" << arg << ")" << std::endl; return "OK";
   };

   command_map["PLAY"] = [this](EventHandler &, const std::string &arg) -> std::string {
      try
      {
         if (remote)
         {
            remote->play(arg);
            return "OK";
         }
         else
            return "NYET";
      }
      catch (const std::exception &e)
      {
         std::cerr << e.what() << std::endl;
         return "ERROR";
      }
   };

   command_map["STOP"] = [this](EventHandler &, const std::string &) -> std::string {
      return plain_action(std::bind(&Remote::stop, remote));
   };

   command_map["PAUSE"] = [this](EventHandler &, const std::string &) -> std::string {
      return plain_action(std::bind(&Remote::pause, remote));
   };

   command_map["UNPAUSE"] = [this](EventHandler &, const std::string &) -> std::string {
      return plain_action(std::bind(&Remote::unpause, remote));
   };

   command_map["SEEK"] = [this](EventHandler &, const std::string &arg) -> std::string {
      int time = std::strtol(arg.c_str(), nullptr, 0);
      std::cerr << "Time: " << time << std::endl;
      return plain_action(std::bind(&Remote::seek, remote, time));
   };

   command_map["POS"] = [this](EventHandler &, const std::string &arg) -> std::string {
      try
      {
         auto pos = remote->pos();
         return stringify(static_cast<int>(pos.first), " ", static_cast<int>(pos.second));
      }
      catch (const std::exception &e)
      {
         std::cerr << e.what() << std::endl;
         return "ERROR";
      }
   };

   command_map["DIE"] = [this](EventHandler &event, const std::string &) -> std::string {
      event.kill();
      return "OK";
   };

   command_map["TITLE"] = [this](EventHandler &, const std::string &) -> std::string {
      return metadata_action(*remote, [](const FF::MediaInfo &info) { return info.title; });
   };

   command_map["ARTIST"] = [this](EventHandler &, const std::string &) -> std::string {
      return metadata_action(*remote, [](const FF::MediaInfo &info) { return info.artist; });
   };

   command_map["ALBUM"] = [this](EventHandler &, const std::string &) -> std::string {
      return metadata_action(*remote, [](const FF::MediaInfo &info) { return info.album; });
   };

   command_map["STATUS"] = [this](EventHandler &, const std::string &) -> std::string {
      return remote->status();
   };
}

void TCPSocket::parse_command(EventHandler &event, const std::string &cmd)
{
   //std::cerr << "Command: " << cmd << std::endl;

   auto split = cmd.find(' ');
   auto first_arg = cmd.find('\"', split);
   auto last_arg = cmd.rfind('\"');

   auto name = cmd.substr(0, split);

   std::string arg;
   if (last_arg > first_arg)
      arg = cmd.substr(first_arg + 1, last_arg - first_arg - 1);

   auto cb = command_map[name];
   if (cb)
   {
      auto reply = cb(event, arg);
      reply += "\r\n";
      if (::write(fd, reply.data(), reply.size()) < static_cast<ssize_t>(reply.size()))
         throw std::runtime_error("Failed writing to command socket.\n");
   }
   else
      throw std::runtime_error(stringify("Unrecognized command: \"", name, "\""));
}

void TCPSocket::parse_commands(EventHandler &event)
{
   if (command_buf.size() > max_cmd_size)
      command_buf.clear();

   for (;;)
   {
      auto first = command_buf.find("\r\n");
      if (first == std::string::npos)
         return;

      auto cmd = command_buf.substr(0, first);
      parse_command(event, cmd);

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

void TCPSocket::set_remote(Remote &remote)
{
   this->remote = &remote;
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

