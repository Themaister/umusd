#include "audio.hpp"
#include "player.hpp"

void Audio::set_media(std::weak_ptr<FF> ff)
{
   this->ff = ff;
}

void Audio::set_remote(Remote &remote)
{
   this->remote = &remote;
}

void Audio::handle(EventHandler &handler)
{
   if (auto tmp = ff.lock())
   {
      auto buf = tmp->decode();
      if (buf.empty())
      {
         try
         {
            remote->next();
         }
         catch(...)
         {}
      }
      else
         write(buf);
   }
}

