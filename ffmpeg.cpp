#include "ffmpeg.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <iostream>

FF::FF(const std::string &path)
   : fctx(nullptr), actx(nullptr), aud_stream(-1), last_pos(0.0f), planar_audio(false)
{
   av_register_all();

   try
   {
      media_info.title = path;

      if (avformat_open_input(&fctx, path.c_str(), nullptr, nullptr) < 0)
         throw std::runtime_error("Failed to open file.\n");

      if (avformat_find_stream_info(fctx, nullptr) < 0)
         throw std::runtime_error("Failed to get stream info.\n");

      resolve_codecs();
      get_media_info();
   } 
   catch(...)
   {
      if (actx)
         avcodec_close(actx);
      if (fctx)
         avformat_close_input(&fctx);
      throw;
   }
}

FF::~FF()
{
   if (actx)
      avcodec_close(actx);
   if (fctx)
      avformat_close_input(&fctx);
}

FF::FF(FF &&ff)
{
   *this = std::move(ff);
}

FF& FF::operator=(FF &&ff)
{
   if (actx)
   {
      avcodec_close(actx);
      actx = nullptr;
   }

   if (fctx)
      avformat_close_input(&fctx);

   std::swap(fctx, ff.fctx);
   std::swap(actx, ff.actx);
   aud_stream = ff.aud_stream;
   media_info = ff.media_info;
   buffer = std::move(ff.buffer);
   planar_audio = ff.planar_audio;

   return *this;
}

FF::MediaInfo::Format FF::fmt_conv(AVSampleFormat fmt)
{
   switch (fmt)
   {
      case AV_SAMPLE_FMT_S16P:
         planar_audio = true;
      case AV_SAMPLE_FMT_S16: // Fallthrough
         return MediaInfo::Format::S16;

      case AV_SAMPLE_FMT_S32P:
         planar_audio = true; // Fallthrough
      case AV_SAMPLE_FMT_S32:
         return MediaInfo::Format::S32;

      case AV_SAMPLE_FMT_FLTP:
         planar_audio = true; // Fallthrough
      case AV_SAMPLE_FMT_FLT:
         return MediaInfo::Format::Float;

      default:
         return MediaInfo::Format::None;
   }
}

void FF::get_metadata(AVDictionary *meta)
{
   auto entry = av_dict_get(meta, "title", nullptr, 0);
   if (entry && entry->value)
      media_info.title = entry->value;

   entry = av_dict_get(meta, "artist", nullptr, 0);
   if (entry && entry->value)
      media_info.artist = entry->value;

   entry = av_dict_get(meta, "album", nullptr, 0);
   if (entry && entry->value)
      media_info.album = entry->value;
}

void FF::get_media_info()
{
   media_info.channels = actx->channels;
   media_info.rate = actx->sample_rate;
   media_info.fmt = fmt_conv(actx->sample_fmt);

   media_info.duration = fctx->streams[aud_stream]->duration * av_q2d(fctx->streams[aud_stream]->time_base);

   get_metadata(fctx->metadata);
   get_metadata(fctx->streams[aud_stream]->metadata);
}

void FF::resolve_codecs()
{
   for (unsigned i = 0; i < fctx->nb_streams; i++)
   {
      if (fctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
      {
         aud_stream = i;
         break;
      }
   }

   if (aud_stream < 0)
      throw std::runtime_error("Failed to locate audio stream.\n");

   AVCodec *codec = avcodec_find_decoder(fctx->streams[aud_stream]->codec->codec_id);
   if (!codec)
      throw std::runtime_error("Failed to locate decoder for stream.\n");

   actx = fctx->streams[aud_stream]->codec;
   if (avcodec_open2(actx, codec, nullptr) < 0)
   {
      actx = nullptr;
      throw std::runtime_error("Failed to open codec.\n");
   }
}

template <typename T>
inline void copy_deplanar(std::uint8_t *out, std::uint8_t **in, int frames, int channels)
{
   T* out_ptr = reinterpret_cast<T*>(out);
   for (int c = 0; c < channels; c++, out_ptr++)
   {
      const T* in_ptr = reinterpret_cast<T*>(in[c]);
      for (int i = 0; i < frames; i++)
         out_ptr[i * channels] = in_ptr[i];
   }
}

const FF::Buffer& FF::decode()
{
   buffer.clear();
   AVPacket pkt;
   AVFrame frame;
   int got_ptr = 0;
   unsigned retry_cnt = 0;

   while (!got_ptr)
   {
      if (av_read_frame(fctx, &pkt) < 0)
      {
         std::cerr << "av_read_frame() failed." << std::endl;
         return buffer;
      }

      if (pkt.stream_index != aud_stream)
      {
         av_free_packet(&pkt);
         continue;
      }

      if (avcodec_decode_audio4(actx, &frame, &got_ptr, &pkt) < 0)
      {
         std::cerr << "avcodec_decode_audio4() failed." << std::endl;
         av_free_packet(&pkt);

         if (retry_cnt++ < 4)
            continue;

         return buffer;
      }

      av_free_packet(&pkt);
   }

   int size = av_samples_get_buffer_size(nullptr, actx->channels,
         frame.nb_samples, actx->sample_fmt, 1);

   if (pkt.pts != static_cast<std::int64_t>(AV_NOPTS_VALUE))
      last_pos = pkt.pts * av_q2d(fctx->streams[aud_stream]->time_base);

   if (planar_audio)
   {
      std::size_t current_size = buffer.size();
      buffer.resize(buffer.size() + size);
      std::uint8_t* out_ptr = buffer.data() + current_size;

      switch (actx->sample_fmt)
      {
         case AV_SAMPLE_FMT_S16P:
            copy_deplanar<int16_t>(out_ptr, frame.data, frame.nb_samples, actx->channels);
            break;

         case AV_SAMPLE_FMT_S32P:
            copy_deplanar<int32_t>(out_ptr, frame.data, frame.nb_samples, actx->channels);
            break;

         case AV_SAMPLE_FMT_FLTP:
            copy_deplanar<float>(out_ptr, frame.data, frame.nb_samples, actx->channels);
            break;

         default:
            break;
      }
   }
   else
      buffer.insert(buffer.end(), frame.data[0], frame.data[0] + size);

   return buffer;
}

const FF::MediaInfo& FF::info() const
{
   return media_info;
}

float FF::pos() const
{
   return last_pos;
}

bool FF::seek(float pos)
{
   int flags = (pos < last_pos) ? AVSEEK_FLAG_BACKWARD : 0;

   double seek_to = pos / av_q2d(fctx->streams[aud_stream]->time_base);
   if (seek_to < 0.0)
      seek_to = 0.0;

   if (av_seek_frame(fctx, aud_stream, seek_to, flags) < 0)
      return false;

   last_pos = pos;
   avcodec_flush_buffers(actx);
   return true;
}

