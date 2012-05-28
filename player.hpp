#ifndef PLAYER_HPP__
#define PLAYER_HPP__

#include <memory>

#include "oss.hpp"
#include "ffmpeg.hpp"
#include "tcpcommand.hpp"
#include "eventhandler.hpp"

class Remote
{
   public:
      virtual void play(const std::string &path) = 0;
      virtual void stop() = 0;

      virtual void pause() = 0;
      virtual void unpause() = 0;

      virtual float pos() const = 0;
      virtual void seek(float pos) = 0;
};

class Player : public Remote
{
   public:
      Player();
      void run();

      void play(const std::string &path);
      void stop();
      void pause();
      void unpause();
      float pos() const;
      void seek(float pos);

   private:
      std::shared_ptr<TCPCommand> cmd;
      std::unique_ptr<EventHandler> event;
      std::shared_ptr<Audio> dev;
      std::shared_ptr<FF> ff;
};

#endif

