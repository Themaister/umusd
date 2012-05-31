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
   {
      if (!queue.current.empty())
      {
         queue.prev.push_back(std::move(queue.current));
         if (queue.prev.size() > 4096)
            queue.prev.erase(queue.prev.begin() + queue.prev.size() - 4096, queue.prev.end());
      }

      queue.current = path;
   }

   if (queue.current.empty())
   {
      if (queue.next.empty())
         throw std::logic_error("No files to play in queue ...\n");

      queue.current = std::move(queue.next.front());
      queue.next.pop_front();
   }

   ff = std::make_shared<FF>(queue.current);
   dev->set_media(ff);
}

void Player::play_audio()
{
   auto info = ff->info();
   dev->init(info.channels, info.rate, dev->default_device());
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
   {
      std::cerr << "Clearing queue ..." << std::endl;
      queue.prev.clear();
      queue.next.clear();
   }
   else
   {
      std::cerr << "Queueing up: " << path << std::endl;
      queue.next.push_back(path);
   }
}

void Player::stop()
{
   dev->stop();
   event->remove(*dev);
   ff.reset();
}

void Player::prev()
{
   if (queue.prev.empty())
      throw std::logic_error("No files in prev queue.");

   stop();

   if (!queue.current.empty())
      queue.next.push_front(std::move(queue.current));

   auto new_song = std::move(queue.prev.back());
   queue.prev.pop_back();
   play(new_song);
}

void Player::next()
{
   FF::MediaInfo old_info;
   if (ff)
      old_info = ff->info();

   if (queue.next.empty())
      throw std::logic_error("No files in next queue.");

   auto new_path = std::move(queue.next.front());
   queue.next.pop_front();
   play_media(new_path);

   auto &new_info = ff->info();

   // Attempt gapless
   if (old_info.channels != new_info.channels || old_info.rate != new_info.rate || !ff)
   {
      stop();
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

