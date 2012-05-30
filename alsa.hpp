#ifndef ALSA_HPP__
#define ALSA_HPP__

#include "audio.hpp"
#include <asoundlib.h>
#include <sys/poll.h>

class ALSA : public Audio
{
   public:
      ALSA();
      ~ALSA();

      std::string default_device() const;
      void init(unsigned channels, unsigned rate, const std::string &dev);
      void write(const FF::Buffer &buf);
      void stop();

      void handle(EventHandler &handler);

      bool active() const;

      EventHandled::PollList pollfds() const;

   private:
      snd_pcm_t *pcm;
      std::vector<struct pollfd> fds;
};

#endif

