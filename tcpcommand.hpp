#ifndef TCPCOMMAND_HPP__
#define TCPCOMMAND_HPP__

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include "command.hpp"

class EventHandler;

class TCPSocket : public Command
{
   public:
      explicit TCPSocket(int fd);
      ~TCPSocket();

      void operator=(const TCPSocket &) = delete;

      TCPSocket& operator=(TCPSocket &&tcp);
      TCPSocket(TCPSocket &&tcp);

      bool dead() const;
      EventHandled::PollList pollfds() const;
      void handle(EventHandler &handler);

   private:
      int fd;
      bool is_dead;

      void kill_sock();
      std::string command_buf;

      void flush_buffer();
      void parse_commands(EventHandler &handler);

      void write_all(EventHandler &handler, std::string &&str);

      std::shared_ptr<SocketReply> reply;
};

class TCPCommand : public EventHandled
{
   public:
      explicit TCPCommand(std::uint16_t port);
      void operator=(const TCPCommand &) = delete;

      TCPCommand& operator=(TCPCommand &&tcp);
      TCPCommand(TCPCommand &&tcp);

      EventHandled::PollList pollfds() const;
      void handle(EventHandler &handler);

      void set_remote(Remote &remote);

   private:
      int fd;
      Remote *remote;
      std::vector<std::shared_ptr<TCPSocket>> connections;
};

#endif

