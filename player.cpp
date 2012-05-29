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
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   auto info = ff->info();
   if (!dev->active())
   {
      dev->init(info.channels, info.rate, dev->default_device());
      event->add(dev);
   }
}

float Player::pos() const
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   return ff->pos();
}

void Player::seek(float pos)
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.\n");

   ff->seek(pos);
}

