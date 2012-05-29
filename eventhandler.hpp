#ifndef EVENTHANDLER_HPP__
#define EVENTHANDLER_HPP__

#include <functional>
#include <map>
#include <memory>
#include <list>
#include <sys/epoll.h>

class Remote;
class EventHandler;
class EventHandled;

class EventHandled
{
   public:
      struct FD
      {
         int fd;
         unsigned events;
      };

      typedef std::list<FD> PollList;

      virtual PollList pollfds() const = 0;
      virtual void handle(EventHandler &handler) = 0;
      virtual void set_remote(Remote &) {};
};

class EventHandler
{
   public:
      EventHandler();
      ~EventHandler();

      void add(std::weak_ptr<EventHandled> fd);
      void remove(const EventHandled &fd);

      void kill();
      bool wait();

   private:
      int epfd;
      bool killed;
      std::map<int, std::function<void ()>> cb_map;
};

#endif

