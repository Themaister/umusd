#include "oss.hpp"
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <stdexcept>

OSS::OSS() : fd(-1) {}

OSS::~OSS()
{
   stop();
}

std::string OSS::default_device() const
{
   return "/dev/dsp";
}

bool OSS::active() const
{
   return fd >= 0;
}

void OSS::init(unsigned channels, unsigned rate, const std::string &dev)
{
   stop();

   fd = open(dev.c_str(), O_WRONLY);
   if (fd < 0)
      throw std::runtime_error("Failed to open OSS device.\n");

   int tmp = channels;
   if (ioctl(fd, SNDCTL_DSP_CHANNELS, &tmp) < 0 || tmp != static_cast<int>(channels))
      throw std::runtime_error("Failed to set number of channels.\n");

   tmp = rate;
   if (ioctl(fd, SNDCTL_DSP_SPEED, &tmp) < 0 || tmp != static_cast<int>(rate))
      throw std::runtime_error("Failed to set sample rate.\n");

   tmp = AFMT_S16_LE;
   if (ioctl(fd, SNDCTL_DSP_SETFMT, &tmp) < 0 || tmp != AFMT_S16_LE)
      throw std::runtime_error("Failed to set sample format.\n");
}

void OSS::stop()
{
   if (fd >= 0)
   {
      close(fd);
      fd = -1;
   }
}

void OSS::write(const FF::Buffer &buf)
{
   const std::uint8_t *data = buf.data();
   std::size_t to_write = buf.size();

   while (to_write)
   {
      ssize_t ret = ::write(fd, data, to_write);
      if (ret <= 0)
         throw std::runtime_error("Failed to write data.\n");

      data += ret;
      to_write -= ret;
   }
}

EventHandled::PollList OSS::pollfds() const
{
   return {{fd, EPOLLOUT}};
}

