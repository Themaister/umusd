#include "ffmpeg.hpp"
#include <cstring>
#include <stdexcept>
#include <algorithm>

FF::FF(const std::string &path)
   : fctx(nullptr), actx(nullptr), aud_stream(-1), last_pos(0.0f)
{
   av_register_all();

   try
   {
      if (avformat_open_input(&fctx, path.c_str(), nullptr, nullptr) < 0)
         throw std::runtime_error("Failed to open file.\n");

      if (avformat_find_stream_info(fctx, nullptr) < 0)
         throw std::runtime_error("Failed to get stream info.\n");

      resolve_codecs();
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
   {
      avformat_close_input(&fctx);
      fctx = nullptr;
   }

   std::swap(fctx, ff.fctx);
   std::swap(actx, ff.actx);
   aud_stream = ff.aud_stream;
   media_info = ff.media_info;
   buffer = std::move(ff.buffer);

   return *this;
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

   media_info.channels = actx->channels;
   media_info.rate = actx->sample_rate;
}

const FF::Buffer& FF::decode()
{
   buffer.clear();
   AVPacket pkt;
   AVFrame frame;
   int got_ptr = 0;

   while (!got_ptr)
   {
      if (av_read_frame(fctx, &pkt) < 0)
         return buffer;

      if (pkt.stream_index != aud_stream)
      {
         av_free_packet(&pkt);
         continue;
      }

      if (avcodec_decode_audio4(actx, &frame, &got_ptr, &pkt) < 0)
      {
         av_free_packet(&pkt);
         return buffer;
      }

      av_free_packet(&pkt);
   }

   int size = av_samples_get_buffer_size(nullptr, actx->channels,
         frame.nb_samples, actx->sample_fmt, 1);

   if (pkt.pts != static_cast<std::int64_t>(AV_NOPTS_VALUE))
      last_pos = pkt.pts * av_q2d(fctx->streams[aud_stream]->time_base);

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

