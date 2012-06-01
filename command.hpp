#ifndef COMMAND_HPP__
#define COMMAND_HPP__

#include "eventhandler.hpp"
#include <string>
#include <map>
#include <functional>
#include <vector>
#include <cstddef>

class EventHandler;
class Remote;

class Command : public EventHandled
{
   public:
      Command();
      void set_remote(Remote &remote);

   protected:
      std::string parse_command(EventHandler &handler, const std::string &cmd);
      Remote *remote;

   private:
      void init_command_map();

      std::map<std::string,
         std::function<std::string (EventHandler &, std::vector<std::string>)>> command_map;
};

class SocketReply : public EventHandled
{
   public:
      SocketReply(int fd,
            std::string &&data, std::function<void (bool)> end_cb);

      void operator=(const SocketReply &) = delete;

      void handle(EventHandler &handler);
      EventHandled::PollList pollfds() const;

   private:
      int fd;

      std::string data;
      std::size_t ptr;
      std::function<void (bool)> end_cb;
};

#endif

