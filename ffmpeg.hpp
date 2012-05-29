#ifndef FFMPEG_HPP__
#define FFMPEG_HPP__

#define __STDC_CONSTANT_MACROS

#include <string>
#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class FF
{
   public:
      FF(const std::string &path);
      ~FF();
      void operator=(const FF&) = delete;

      FF(FF &&ff);
      FF& operator=(FF &&ff);

      struct MediaInfo
      {
         unsigned channels;
         unsigned rate;

         float duration;

         std::string title, artist, album;
      };

      const MediaInfo &info() const;
      float pos() const;

      bool seek(float pos);

      typedef std::vector<std::uint8_t> Buffer;
      const Buffer& decode();

   private:
      AVFormatContext *fctx;
      AVCodecContext *actx;

      int aud_stream;
      float last_pos;
      MediaInfo media_info;

      Buffer buffer;

      void resolve_codecs();
      void get_media_info();
      void get_metadata(AVDictionary *meta);
};

#endif

