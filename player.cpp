#include "player.hpp"
#include <stdexcept>
#include <iostream>

Player::Player()
{
   cmd = std::make_shared<TCPCommand>(42878);
   cmd->set_remote(*this);

   event = std::unique_ptr<EventHandler>(new EventHandler);
   dev = std::shared_ptr<Audio>(new ALSA);
   dev->set_remote(*this);

   event->add(cmd);
}

void Player::run()
{
   while (event->wait());
}

void Player::play_media(const std::string &path)
{
   if (!path.empty())
      queue.current(path);

   ff = std::make_shared<FF>(queue.current());
   dev->set_media(ff);
}

void Player::play_audio()
{
   auto info = ff->info();
   dev->init(info.channels, info.rate, info.fmt, dev->default_device());
   event->add(dev);
}

void Player::play(const std::string& path)
{
   play_media(path);
   play_audio();
}

void Player::add(const std::string& path)
{
   if (path.empty())
      queue.clear();
   else
      queue.add(path);
}

void Player::stop()
{
   dev->stop();
   event->remove(*dev);
   ff.reset();
}

void Player::prev()
{
   queue.prev();

   stop();
   play();
}

void Player::next()
{
   FF::MediaInfo old_info{};
   if (ff)
      old_info = ff->info();

   queue.next();
   play_media();

   auto &new_info = ff->info();

   // Attempt gapless
   if (old_info.channels != new_info.channels ||
         old_info.rate != new_info.rate ||
         old_info.fmt != new_info.fmt ||
         !ff ||
         !dev->active())
   {
      event->remove(*dev);
      play_audio();
   }
}

const FF::MediaInfo Player::media_info() const
{
   if (!ff)
      throw std::logic_error("FFmpeg file not loaded.");

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
      dev->init(info.channels, info.rate, info.fmt, dev->default_device());
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

