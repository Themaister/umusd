#include "alsa.hpp"
#include <stdexcept>

ALSA::ALSA() : pcm(nullptr)
{}

ALSA::~ALSA()
{
   stop();
}

void ALSA::stop()
{
   if (pcm)
   {
      snd_pcm_drop(pcm);
      snd_pcm_close(pcm);
   }

   pcm = nullptr;
   fds.clear();
}

bool ALSA::active() const
{
   return pcm;
}

std::string ALSA::default_device() const
{
   return "default";
}

#define TRY(action, error) { \
   if ((action) < 0) \
      throw std::runtime_error(error); \
}

void ALSA::init(unsigned channels, unsigned rate,
      FF::MediaInfo::Format fmt,
      const std::string &dev)
{
   stop();

   try
   {
      TRY(snd_pcm_open(&pcm, dev.c_str(), SND_PCM_STREAM_PLAYBACK, 0),
            "Failed to open device.\n");

      snd_pcm_hw_params_t *params;
      snd_pcm_hw_params_alloca(&params);

      TRY(snd_pcm_hw_params_any(pcm, params),
            "Failed to set initial params.\n");

      TRY(snd_pcm_hw_params_set_access(pcm, params,
               SND_PCM_ACCESS_RW_INTERLEAVED),
            "Failed to set RW access.\n");

      snd_pcm_format_t format;
      switch (fmt)
      {
         case FF::MediaInfo::Format::S16:
            format = SND_PCM_FORMAT_S16;
            break;
         case FF::MediaInfo::Format::S32:
            format = SND_PCM_FORMAT_S32;
            break;
         default:
            format = SND_PCM_FORMAT_UNKNOWN;
            break;
      }

      TRY(snd_pcm_hw_params_set_format(pcm, params,
               format),
            "Failed to set sample format.\n");

      TRY(snd_pcm_hw_params_set_channels(pcm, params,
               channels),
            "Failed to set number of channels.\n");

      TRY(snd_pcm_hw_params_set_rate(pcm, params, rate, 0),
            "Failed to set sampling rate.\n");

      unsigned usec = 500000;
      unsigned periods = 4;
      TRY(snd_pcm_hw_params_set_buffer_time_near(pcm, params,
               &usec, nullptr),
            "Failed to set buffer time.\n");

      TRY(snd_pcm_hw_params_set_periods_near(pcm, params,
               &periods, nullptr),
            "Failed to set periods.\n");

      TRY(snd_pcm_hw_params(pcm, params),
            "Failed to install params.\n");

      fds.resize(snd_pcm_poll_descriptors_count(pcm));
      snd_pcm_poll_descriptors(pcm, fds.data(), fds.size());
   }
   catch(...)
   {
      stop();
      throw;
   }
}

void ALSA::write(const FF::Buffer &buf)
{
   auto buffer = buf.data();
   auto size = snd_pcm_bytes_to_frames(pcm, buf.size());

   while (size)
   {
      auto frames = snd_pcm_writei(pcm, buffer, size);
      if (frames == -EPIPE || frames == -EINTR || frames == -ESTRPIPE)
      {
         if (snd_pcm_recover(pcm, frames, 1) < 0)
            throw std::runtime_error("Failed to recover ALSA.\n");
      }
      else if (frames < 0)
         throw std::runtime_error("ALSA failed to write.\n");

      buffer += snd_pcm_frames_to_bytes(pcm, frames);
      size -= frames;
   }
}

EventHandled::PollList ALSA::pollfds() const
{
   EventHandled::PollList list;
   for (auto fd : fds)
   {
      list.push_back({fd.fd,
            (fd.events & POLLIN ? EPOLLIN : 0) |
            (fd.events & POLLOUT ? EPOLLOUT : 0)});
   }

   return list;
}

void ALSA::handle(EventHandler &handler)
{
   if (!pcm)
      return;

   for (auto &fd : fds)
      fd.revents = 0;

   if (poll(fds.data(), fds.size(), 0) < 0)
      throw std::runtime_error("ALSA poll failed.");

   unsigned short revents;
   if (snd_pcm_poll_descriptors_revents(pcm, fds.data(),
            fds.size(), &revents) < 0)
      throw std::runtime_error("ALSA revents failed.\n");

   if (!(revents & POLLOUT))
      return;

   Audio::handle(handler);
}

