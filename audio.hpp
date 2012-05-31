#ifndef AUDIO_HPP__
#define AUDIO_HPP__

#include "ffmpeg.hpp"
#include "eventhandler.hpp"

#include <string>
#include <memory>

class Audio : public EventHandled
{
   public:
      virtual ~Audio() {}

      virtual std::string default_device() const = 0;

      void set_media(std::weak_ptr<FF> ff);

      void set_remote(Remote &remote);
      virtual void handle(EventHandler &handler);

      virtual void init(unsigned channels, unsigned rate, const std::string &dev) = 0;
      virtual void write(const FF::Buffer &buf) = 0;
      virtual void stop() = 0;

      virtual bool active() const = 0;

   protected:
      std::weak_ptr<FF> ff;
      Remote *remote;
};

#endif

