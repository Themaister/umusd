#ifndef COMMAND_HPP__
#define COMMAND_HPP__

#include "eventhandler.hpp"
#include <string>
#include <map>
#include <functional>
#include <vector>

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

#endif

