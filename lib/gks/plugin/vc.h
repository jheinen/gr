#ifndef _VC_H_
#define _VC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#define VC_FLAGS_MOV_HIDPI (1 << 0)

struct frame_t_
{
  unsigned char *data;
  int width, height;
};

struct movie_t_
{
  AVFormatContext *fmt_ctx;
  AVOutputFormat *out_fmt;
  AVCodecContext *cdc_ctx;
  AVStream *video_st;
  AVFrame *frame;
  float t;
  struct SwsContext *sws_ctx;

  unsigned char *gif_scaled_image;
  unsigned char *gif_scaled_image_copy;
  unsigned char *gif_palette;
};

typedef struct movie_t_ *movie_t;
typedef struct frame_t_ *frame_t;

movie_t vc_movie_create(const char *path, int framerate, int bitrate, int width, int height, int flags);
void vc_movie_append_frame(movie_t movie, frame_t frame);
void vc_movie_finish(movie_t movie);

#ifdef __cplusplus
}
#endif

#endif
