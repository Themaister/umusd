#include "eventhandler.hpp"
#include <sys/epoll.h>
#include <unistd.h>

#include <stdexcept>
#include <iostream>

EventHandler::EventHandler() : killed(false)
{
   epfd = epoll_create(16);
   if (epfd < 0)
      throw std::runtime_error("Failed to create epoll.\n");
}

EventHandler::~EventHandler()
{
   close(epfd);
}

void EventHandler::add(std::weak_ptr<EventHandled> handler)
{
   auto fd_ptr = handler.lock();

   for (auto fd : fd_ptr->pollfds())
   {
      struct epoll_event event{};
      event.events = fd.events;
      event.data.fd = fd.fd;

      cb_map[event.data.fd] = [this, handler]() {
         if (auto tmp = handler.lock())
            tmp->handle(*this);
      };

      if (epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event) < 0)
         throw std::runtime_error("Failed to add epoll fd to list.\n");
   }
}

void EventHandler::remove(const EventHandled &handler)
{
   for (auto fd : handler.pollfds())
   {
      cb_map[fd.fd] = {};
      epoll_ctl(epfd, EPOLL_CTL_DEL, fd.fd, nullptr);
   }
}

void EventHandler::kill()
{
   killed = true;
}

bool EventHandler::wait()
{
   struct epoll_event events[16];
   int ret = epoll_wait(epfd, events, 16, -1);
   if (ret < 0)
      throw std::runtime_error("epoll_wait() failed.\n");

   for (int i = 0; i < ret; i++)
   {
      auto cb = cb_map[events[i].data.fd];
      if (cb)
         cb();
   }

   return !killed;
}

