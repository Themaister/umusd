#include "queue.hpp"
#include <stdexcept>

void PlayQueue::current(const std::string &path)
{
   if (path.empty())
      return;

   if (!queue.current.empty())
   {
      queue.prev.push_back(std::move(queue.current));

      if (queue.prev.size() > max_prev_backlog)
      {
         queue.prev.erase(queue.prev.begin() + queue.prev.size() - max_prev_backlog,
               queue.prev.end());
      }
   }

   queue.current = path;
}

const std::string& PlayQueue::current()
{
   if (queue.current.empty())
      next();

   return queue.current;
}

void PlayQueue::prev()
{
   if (queue.prev.empty())
      throw std::logic_error("No files in prev queue.");

   if (!queue.current.empty())
      queue.next.push_front(std::move(queue.current));

   queue.current = std::move(queue.prev.back());
   queue.prev.pop_back();
}

void PlayQueue::next()
{
   if (queue.next.empty())
      throw std::logic_error("No files in next queue.");

   current(std::move(queue.next.front()));
   queue.next.pop_front();
}

void PlayQueue::clear()
{
   queue.prev.clear();
   queue.next.clear();
}

void PlayQueue::add(const std::string &path)
{
   queue.next.push_back(path);
}

