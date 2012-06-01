#ifndef QUEUE_HPP__
#define QUEUE_HPP__

#include <deque>
#include <string>

class PlayQueue
{
   public:
      void current(const std::string &current);
      void add(const std::string &path);
      void clear();

      const std::string &current();

      void prev();
      void next();

   private:
      struct
      {
         std::string current;
         std::deque<std::string> prev, next;
      } queue;

      enum { max_prev_backlog = 4096 };
};

#endif

