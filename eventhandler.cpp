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

void EventHandler::add(std::weak_ptr<EventHandled> fd, int events)
{
   struct epoll_event event = {0};
   event.events = events;

   auto fd_ptr = fd.lock();
   event.data.fd = fd_ptr->pollfd();

   cb_map[event.data.fd] = [this, fd]() {
      if (auto tmp = fd.lock())
         tmp->handle(*this);
   };

   if (epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event) < 0)
      throw std::runtime_error("Failed to add epoll fd to list.\n");
}

void EventHandler::remove(const EventHandled &fd)
{
   cb_map[fd.pollfd()] = {};
   epoll_ctl(epfd, EPOLL_CTL_DEL, fd.pollfd(), nullptr);
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

