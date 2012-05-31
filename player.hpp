#ifndef PLAYER_HPP__
#define PLAYER_HPP__

#include <memory>
#include <utility>
#include <deque>

#include "oss.hpp"
#include "alsa.hpp"
#include "ffmpeg.hpp"
#include "tcpcommand.hpp"
#include "eventhandler.hpp"

class Remote
{
   public:
      virtual void play(const std::string &path) = 0;
      virtual void add(const std::string &path) = 0;
      virtual void stop() = 0;
      virtual void prev() = 0;
      virtual void next() = 0;

      virtual void pause() = 0;
      virtual void unpause() = 0;

      virtual std::pair<float, float> pos() const = 0;
      virtual void seek(float pos) = 0;

      virtual const FF::MediaInfo media_info() const = 0;

      virtual std::string status() const = 0;
};

class Player : public Remote
{
   public:
      Player();
      void run();

      void play(const std::string &path);
      void add(const std::string &path);
      void stop();
      void next();
      void prev();
      void pause();
      void unpause();
      std::pair<float, float> pos() const;
      void seek(float pos);

      virtual const FF::MediaInfo media_info() const;
      std::string status() const;

   private:
      std::shared_ptr<TCPCommand> cmd;
      std::unique_ptr<EventHandler> event;
      std::shared_ptr<Audio> dev;
      std::shared_ptr<FF> ff;

      struct
      {
         std::string current;
         std::deque<std::string> prev, next;
      } queue;
};

#endif

