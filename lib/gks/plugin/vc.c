#if !defined(NO_AV)

#include <stdio.h>

#include "vc.h"
#include "gif.h"
#include "gkscore.h"

static void encode_frame(movie_t movie)
{
  int ret;
  AVPacket pkt = {0};
  av_init_packet(&pkt);
  ret = avcodec_send_frame(movie->cdc_ctx, movie->frame);
  if (ret < 0)
    {
      fprintf(stderr, "Error sending frame %ld for encoding\n", (long)movie->frame->pts);
      return;
    }
  while (ret >= 0)
    {
      ret = avcodec_receive_packet(movie->cdc_ctx, &pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
          return;
        }
      else if (ret < 0)
        {
          fprintf(stderr, "Error during encoding of frame %ld\n", (long)movie->frame->pts);
          return;
        }
      av_packet_rescale_ts(&pkt, movie->cdc_ctx->time_base, movie->video_st->time_base);
      pkt.stream_index = movie->video_st->index;

      ret = av_interleaved_write_frame(movie->fmt_ctx, &pkt);
      av_packet_unref(&pkt);
    }
}

void vc_movie_append_frame(movie_t movie, frame_t frame)
{
  int is_gif = movie->cdc_ctx->pix_fmt == AV_PIX_FMT_PAL8;
  int height = movie->cdc_ctx->height;
  int width = movie->cdc_ctx->width;
  int i;

  if (!movie->sws_ctx)
    {
      int dst_pix_format = movie->cdc_ctx->pix_fmt;
      if (is_gif)
        {
          dst_pix_format = AV_PIX_FMT_RGBA;
        }
      movie->sws_ctx = sws_getContext(frame->width, frame->height, AV_PIX_FMT_RGBA, width, height, dst_pix_format,
                                      SWS_BICUBIC, NULL, NULL, NULL);
      if (!movie->sws_ctx)
        {
          fprintf(stderr, "Could not initialize the conversion context\n");
          return;
        }
    }

  int src_stride[4] = {4 * frame->width, 0, 0, 0};
  const unsigned char *src_slice[4] = {frame->data, 0, 0, 0};

  if (is_gif)
    {
      int dst_stride[4] = {4 * width, 0, 0, 0};
      unsigned char *dst_slice[4] = {movie->gif_scaled_image, 0, 0, 0};

      sws_scale(movie->sws_ctx, src_slice, src_stride, 0, frame->height, dst_slice, dst_stride);

      memcpy(movie->gif_scaled_image_copy, movie->gif_scaled_image, width * height * 4);
      median_cut(movie->gif_scaled_image_copy, movie->gif_palette, width * height, AVPALETTE_COUNT, 4);

      for (i = 0; i < width * height; i++)
        {
          movie->gif_scaled_image[i] =
              color_index_for_rgb(movie->gif_scaled_image + (i * 4), movie->gif_palette, AVPALETTE_COUNT, 4);
        }

      movie->frame->data[0] = movie->gif_scaled_image;
      movie->frame->data[1] = movie->gif_palette;

      movie->frame->linesize[0] = width;
      movie->frame->linesize[1] = 0;
    }
  else
    {
      sws_scale(movie->sws_ctx, src_slice, src_stride, 0, frame->height, movie->frame->data, movie->frame->linesize);
    }
  encode_frame(movie);
  movie->frame->pts++;
}

movie_t vc_movie_create(const char *path, int framerate, int bitrate, int width, int height, int flags)
{
  const AVCodec *codec;
  int ret;

  av_log_set_level(AV_LOG_QUIET);

#if (LIBAVUTIL_VERSION_MAJOR < 56)
  /* av_register_all is deprecated and unnecessary since ffmpeg version 4.0 */
  av_register_all();
#endif

  movie_t movie = (movie_t)gks_malloc(sizeof(struct movie_t_));

  const char *format_name = NULL;
  if (strlen(path) >= 3 && strcmp(path + strlen(path) - 3, "mov") == 0)
    {
      format_name = "mov";
    }

  avformat_alloc_output_context2(&movie->fmt_ctx, NULL, format_name, path);
  if (!movie->fmt_ctx || movie->fmt_ctx->oformat->video_codec == AV_CODEC_ID_NONE)
    {
      fprintf(stderr, "Failed to allocate the output context\n");
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }
  movie->out_fmt = movie->fmt_ctx->oformat;
  codec = avcodec_find_encoder(movie->out_fmt->video_codec);
  if (!codec && movie->out_fmt->video_codec == 12)
    {
      codec = avcodec_find_encoder_by_name("libopenh264");
    }
  if (!codec)
    {
      fprintf(stderr, "Could not find encoder for '%s'\n", avcodec_get_name(movie->out_fmt->video_codec));
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  movie->video_st = avformat_new_stream(movie->fmt_ctx, codec);
  if (!movie->video_st)
    {
      fprintf(stderr, "Could not allocate video stream\n");
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  if (flags & VC_FLAGS_MOV_HIDPI)
    {
      width *= 2;
      height *= 2;
    }

  movie->cdc_ctx = avcodec_alloc_context3(codec);

  movie->cdc_ctx->bit_rate = bitrate;
  movie->cdc_ctx->width = width;
  movie->cdc_ctx->height = height;
  movie->cdc_ctx->time_base = (AVRational){1, framerate};
  movie->cdc_ctx->framerate = (AVRational){framerate, 1};

  if (movie->fmt_ctx->oformat->video_codec == AV_CODEC_ID_GIF)
    {
      movie->cdc_ctx->pix_fmt = AV_PIX_FMT_PAL8;
      movie->gif_palette = (unsigned char *)gks_malloc(AVPALETTE_SIZE);
      movie->gif_scaled_image = (unsigned char *)gks_malloc(width * height * 4);
      movie->gif_scaled_image_copy = (unsigned char *)gks_malloc(width * height * 4);
    }
  else
    {
      movie->cdc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    }
  /* Some formats want stream headers to be separate. */
  if (movie->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    {
      movie->cdc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

  movie->video_st->time_base = movie->cdc_ctx->time_base;
  movie->video_st->r_frame_rate = movie->cdc_ctx->framerate;

  ret = avcodec_open2(movie->cdc_ctx, codec, NULL);
  if (ret < 0)
    {
      fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  ret = avcodec_parameters_from_context(movie->video_st->codecpar, movie->cdc_ctx);
  if (ret < 0)
    {
      fprintf(stderr, "Could not set codec parameters\n");
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  movie->frame = av_frame_alloc();
  if (!movie->frame)
    {
      fprintf(stderr, "Could not allocate video frame\n");
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  movie->frame->format = movie->cdc_ctx->pix_fmt;
  movie->frame->width = movie->cdc_ctx->width;
  movie->frame->height = movie->cdc_ctx->height;
  movie->frame->pts = 0;

  ret = av_frame_get_buffer(movie->frame, 32);
  if (ret < 0)
    {
      fprintf(stderr, "Could not allocate frame data.\n");
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  if (!(movie->out_fmt->flags & AVFMT_NOFILE))
    {
      ret = avio_open(&movie->fmt_ctx->pb, path, AVIO_FLAG_WRITE);
      if (ret < 0)
        {
          fprintf(stderr, "Error occurred while opening output file '%s': %s\n", path, av_err2str(ret));
          vc_movie_finish(movie);
          gks_free(movie);
          return NULL;
        }
    }

  AVDictionary *muxer_options = NULL;
  if (flags & VC_FLAGS_MOV_HIDPI)
    {
      av_dict_set(&muxer_options, "movflags", "write_pixeldensity", 0);
    }
  ret = avformat_write_header(movie->fmt_ctx, &muxer_options);
  if (ret < 0)
    {
      fprintf(stderr, "Error occurred while writing video header: %s\n", av_err2str(ret));
      vc_movie_finish(movie);
      gks_free(movie);
      return NULL;
    }

  return movie;
}

void vc_movie_finish(movie_t movie)
{
  if (movie->frame)
    {
      /* drain encoder */
      av_frame_free(&movie->frame);
      movie->frame = NULL;
      encode_frame(movie);
    }

  if (movie->sws_ctx)
    {
      sws_freeContext(movie->sws_ctx);
      movie->sws_ctx = NULL;
    }

  gks_free(movie->gif_palette);
  gks_free(movie->gif_scaled_image);
  gks_free(movie->gif_scaled_image_copy);

  if (movie->fmt_ctx && movie->cdc_ctx)
    {
      av_write_trailer(movie->fmt_ctx);
      avcodec_close(movie->cdc_ctx);

      if (!(movie->out_fmt->flags & AVFMT_NOFILE))
        {
          avio_closep(&movie->fmt_ctx->pb);
        }

      avformat_free_context(movie->fmt_ctx);
      avcodec_free_context(&movie->cdc_ctx);
    }
}

#endif
