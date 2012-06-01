#include "command.hpp"
#include "utils.hpp"
#include "player.hpp"
#include <stdexcept>
#include <iostream>
#include <signal.h>

Command::Command() : remote(nullptr)
{
   init_command_map();
}

std::string Command::parse_command(EventHandler &event, const std::string &cmd)
{
   auto split = cmd.find(' ');
   auto first_arg = cmd.find('\"', split);
   auto last_arg = cmd.rfind('\"');

   auto name = cmd.substr(0, split);

   std::string arg;
   if (last_arg > first_arg)
      arg = cmd.substr(first_arg + 1, last_arg - first_arg - 1);

   auto &cb = command_map[name];
   if (cb)
      return cb(event, string_split(arg, "\n"));
   else
      throw std::runtime_error(stringify("Unrecognized command: \"", name, "\""));
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

void Command::set_remote(Remote &remote)
{
   this->remote = &remote;
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

void Command::init_command_map()
{
   command_map["NOOP"] = [](EventHandler &, std::vector<std::string> arg) -> std::string {
      std::cerr << "NOOP begin" << std::endl;
      std::cerr << string_join(arg, "\n") << std::endl << "end" << std::endl;
      return "OK";
   };

   command_map["PLAY"] = [this](EventHandler &, std::vector<std::string> arg) -> std::string {
      if (arg.empty())
         arg.push_back("");

      try
      {
         remote->play(arg.front());
         arg.erase(arg.begin());
         for (auto &str : arg)
            remote->add(str);
         return "OK";
      }
      catch(const std::exception &e)
      {
         std::cerr << e.what() << std::endl;
         return "ERROR";
      }
   };

   command_map["QUEUE"] = [this](EventHandler &, std::vector<std::string> arg) -> std::string {
      if (arg.empty())
         arg.push_back("");

      try
      {
         for (auto &str : arg)
         remote->add(str);
         return "OK";
      }
      catch(const std::exception &e)
      {
         std::cerr << e.what() << std::endl;
         return "ERROR";
      }
   };

   command_map["NEXT"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return plain_action(std::bind(&Remote::next, remote));
   };

   command_map["PREV"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return plain_action(std::bind(&Remote::prev, remote));
   };

   command_map["STOP"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return plain_action(std::bind(&Remote::stop, remote));
   };

   command_map["PAUSE"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return plain_action(std::bind(&Remote::pause, remote));
   };

   command_map["UNPAUSE"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return plain_action(std::bind(&Remote::unpause, remote));
   };

   command_map["SEEK"] = [this](EventHandler &, std::vector<std::string> arg) -> std::string {
      if (arg.empty())
         throw std::logic_error("SEEK requires argument.");

      int time = std::strtol(arg[0].c_str(), nullptr, 0);
      return plain_action(std::bind(&Remote::seek, remote, time));
   };

   command_map["POS"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
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

   command_map["DIE"] = [this](EventHandler &event, std::vector<std::string>) -> std::string {
      event.kill();
      return "OK";
   };

   command_map["TITLE"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return metadata_action(*remote, [](const FF::MediaInfo &info) { return info.title; });
   };

   command_map["ARTIST"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return metadata_action(*remote, [](const FF::MediaInfo &info) { return info.artist; });
   };

   command_map["ALBUM"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return metadata_action(*remote, [](const FF::MediaInfo &info) { return info.album; });
   };

   command_map["STATUS"] = [this](EventHandler &, std::vector<std::string>) -> std::string {
      return remote->status();
   };
}

SocketReply::SocketReply(int fd,
      std::string &&data, std::function<void (bool)> end_cb)
   : fd(fd), data(std::move(data)), ptr(0), end_cb(end_cb)
{
   struct sigaction sig{};
   sig.sa_handler = SIG_IGN;
   sigaction(SIGPIPE, &sig, nullptr);
}

EventHandled::PollList SocketReply::pollfds() const
{
   return {{fd, EPOLLOUT}};
}

void SocketReply::handle(EventHandler &handler)
{
   size_t to_write = data.size() - ptr;

   ssize_t ret = write(fd, data.data() + ptr, to_write);
   if (ret <= 0)
   {
      handler.remove(*this);
      end_cb(false);
   }
   else
      ptr += ret;

   if (ptr >= data.size())
   {
      handler.remove(*this);
      end_cb(true);
   }
}

