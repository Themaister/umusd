#include "player.hpp"
#include <stdexcept>

Player::Player()
{
   cmd = std::make_shared<TCPCommand>(42878);
   cmd->set_remote(*this);

   event = std::unique_ptr<EventHandler>(new EventHandler);
   dev = std::shared_ptr<Audio>(new ALSA);
   event->add(cmd);
}

void Player::run()
{
   while (event->wait());
}

void Player::play(const std::string& path)
{
   ff = std::make_shared<FF>(path);
   dev->set_media(ff);

   auto info = ff->info();
   dev->init(info.channels, info.rate, dev->default_device());
   event->add(dev);
}

void Player::stop()
{
   dev->stop();
   event->remove(*dev);
   ff.reset();
}

const FF::MediaInfo Player::media_info() const
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   return ff->info();
}

void Player::pause()
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   if (dev->active())
   {
      event->remove(*dev);
      dev->stop();
   }
}

void Player::unpause()
{
   auto info = media_info();
   if (!dev->active())
   {
      dev->init(info.channels, info.rate, dev->default_device());
      event->add(dev);
   }
}

std::pair<float, float> Player::pos() const
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   return { ff->pos(), ff->info().duration };
}

void Player::seek(float pos)
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   ff->seek(pos);
}

std::string Player::status() const
{
   if (ff && dev->active())
      return "PLAYING";
   else if (ff && !dev->active())
      return "PAUSED";
   else
      return "STOPPED";
}

