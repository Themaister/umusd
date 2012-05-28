#ifndef TCPCOMMAND_HPP__
#define TCPCOMMAND_HPP__

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include "eventhandler.hpp"

class EventHandler;
class Remote;

class TCPSocket : public EventHandled
{
   public:
      explicit TCPSocket(int fd);
      ~TCPSocket();

      void operator=(const TCPSocket &) = delete;

      TCPSocket& operator=(TCPSocket &&tcp);
      TCPSocket(TCPSocket &&tcp);

      bool dead() const;
      int pollfd() const override;
      void handle(EventHandler &handler) override;
      void set_remote(Remote &remote) override;

   private:
      int fd;
      Remote *remote;
      bool is_dead;

      void kill_sock();
      std::string command_buf;

      void flush_buffer();
      void parse_commands(EventHandler &handler);
      void parse_command(EventHandler &handler, const std::string &cmd);

      enum { max_cmd_size = 4096 };

      std::map<std::string,
         std::function<std::string (EventHandler &,
               const std::string &)>> command_map;

      void init_command_map();
};

class TCPCommand : public EventHandled
{
   public:
      explicit TCPCommand(std::uint16_t port);
      void operator=(const TCPCommand &) = delete;

      TCPCommand& operator=(TCPCommand &&tcp);
      TCPCommand(TCPCommand &&tcp);

      int pollfd() const override;
      void handle(EventHandler &handler) override;

      void set_remote(Remote &remote) override;

   private:
      int fd;
      std::vector<std::shared_ptr<TCPSocket>> connections;
      Remote *remote;
};

#endif

