#include "audio.hpp"

void Audio::set_media(std::weak_ptr<FF> ff)
{
   this->ff = ff;
}

void Audio::handle(EventHandler &handler)
{
   if (auto tmp = ff.lock())
   {
      auto buf = tmp->decode();
      if (buf.empty())
      {
         handler.remove(*this);
         stop();
      }
      else
         write(buf);
   }
}

