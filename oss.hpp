#ifndef OSS_HPP__
#define OSS_HPP__

#include "audio.hpp"

class OSS : public Audio
{
   public:
      OSS();
      ~OSS();

      std::string default_device() const;
      void init(unsigned channels, unsigned rate, const std::string &dev);
      void write(const FF::Buffer &buf);
      void stop();

      bool active() const;

      int pollfd() const;

   private:
      int fd;
};

#endif

