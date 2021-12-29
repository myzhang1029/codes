/* Resample and convert any audio to mono 8kHz 8-bit PCM. */
/*
 *  8bit.c
 *  Copyright (C) 2021 Zhang Maiyun <myzhang1029@hotmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <getopt.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

/* Verbose printer */
#define v_printf(...)                                                          \
    do                                                                         \
    {                                                                          \
        if (app_data.if_verbose)                                               \
            fprintf(stderr, __VA_ARGS__);                                      \
    } while (0)

#define averror(message) av_log(NULL, AV_LOG_ERROR, "%s\n", (message))

#define DST_SAMPLE_RATE 8000

static struct app_data
{
    bool if_verbose;
    char *input_url;
    FILE *output;
} app_data;

static const struct option LONG_OPTIONS[] = {
    {"help", no_argument, NULL, 'h'},
    {"verbose", no_argument, NULL, 'v'},
    {NULL, 0, NULL, 0},
};

static const char *FILTER =
    "pan=1c|c0=0.5*c0+0.5*c1,aresample=resampler=soxr:sample_rate=8000,aformat="
    "sample_fmts=u8:channel_layouts=mono";

static void usage(const char *argv0)
{
    printf("Usage: %s [OPTION]... INPUT OUTPUT\n", argv0);
    puts("Resample and convert any audio to mono 8kHz 8-bit PCM.\n\n"
         "  -v, --verbose   show detailed information to stderr\n"
         "  -h, --help      display this help and exit");
    exit(0);
}

static void arg_error(const char *message, const char *argv0)
{
    fprintf(stderr, "%s: %s\nTry '%s --help' for more information.\n", argv0,
            message, argv0);
    exit(1);
}

static void parse_args(int argc, char **argv)
{
    while (true)
    {
        int option_index = 0;
        int c;
        c = getopt_long(argc, argv, "hv", LONG_OPTIONS, &option_index);
        if (c == -1)
        {
            if (optind + 2 == argc)
            {
                app_data.input_url = argv[optind];
                if (strcmp(argv[optind + 1], "-") == 0)
                    app_data.output = stdout;
                else
                    app_data.output = fopen(argv[optind + 1], "wb");
            }
            else
                arg_error("expecting an input file and an output file.",
                          argv[0]);
            break;
        }
        switch (c)
        {
            case 'h':
                usage(argv[0]);
            case 'v':
                app_data.if_verbose = true;
                break;
            case '?':
                /* Error message printed by getopt */
                exit(1);
            default:
                abort();
        }
    }
}

#if 0
/* Find the stream index of the n-th audio stream, where n=app_data.stream_index
 */
AVStream *find_audio_stream(AVFormatContext *fmt_ctx)
{
    long found_astreams = -1;
    unsigned int idx;
    for (idx = 0; idx < fmt_ctx->nb_streams; idx++)
    {
        if (fmt_ctx->streams[idx]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            v_printf("Ignoring video stream %u\n", idx);
        if (fmt_ctx->streams[idx]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            found_astreams += 1;
            if (found_astreams == (long)app_data.stream_index)
            {
                v_printf("Strean #%u is audio stream #%u\n", idx,
                         app_data.stream_index);
                return fmt_ctx->streams[idx];
            }
            v_printf("Ignoring audeo stream %u\n", idx);
        }
    }
    av_log(NULL, AV_LOG_ERROR, "couldn't find an audio stream of index %u", app_data.stream_index);
    return NULL;
}
#endif

int open_audio_stream(AVFormatContext **pfmt_ctx, AVCodecContext **pavc_ctx,
                      AVStream **past)
{
    AVCodec *avc;
    AVStream *ast;
    AVFormatContext *fmt_ctx;
    AVCodecContext *avc_ctx;
    int stream_idx;
    /* Open input file. URLs are supported by FFmpeg */
    if (avformat_open_input(&fmt_ctx, app_data.input_url, NULL, NULL) != 0)
    {
        averror("failed to open input file");
        return 2;
    }
    /* Parse stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        averror("cannot find stream information");
        avformat_close_input(&fmt_ctx);
        return 3;
    }

    /* Dump format information */
    if (app_data.if_verbose)
        av_dump_format(fmt_ctx, 0, app_data.input_url, 0);

    /* Find an audio stream that's available to use */
    if ((stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1,
                                          &avc, 0)) < 0)
    {
        averror("failed to find an audio stream");
        avformat_close_input(&fmt_ctx);
        return 3;
    }
    ast = fmt_ctx->streams[stream_idx];
    if (!(avc_ctx = avcodec_alloc_context3(avc)))
    {
        averror("could not allocate avcodec context");
        avformat_close_input(&fmt_ctx);
        return 3;
    }
    if (avcodec_parameters_to_context(avc_ctx, ast->codecpar) < 0)
    {
        averror("could not create avcodec context");
        avcodec_free_context(&avc_ctx);
        avformat_close_input(&fmt_ctx);
        return 3;
    }
    if (avcodec_open2(avc_ctx, avc, NULL) < 0)
    {
        averror("could not open audio decoder");
        avcodec_free_context(&avc_ctx);
        avformat_close_input(&fmt_ctx);
        return 3;
    }
    /* Not touching the output unless everything succeeds */
    *pfmt_ctx = fmt_ctx;
    *past = ast;
    *pavc_ctx = avc_ctx;
    return 0;
}

/* Create desired audio filters and create I/O buffers */
static int create_filters(AVStream *ast, AVCodecContext *avc_ctx,
                          AVFilterContext **pbuffersrc_ctx,
                          AVFilterContext **pbuffersink_ctx)
{
    char filter_args[512];
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVRational time_base = ast->time_base;
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
    {
        averror("could not allocate avfilter graph");
        return 3;
    }

    /* Input buffer */
    if (!avc_ctx->channel_layout)
    {
        avc_ctx->channel_layout =
            av_get_default_channel_layout(avc_ctx->channels);
    }
    snprintf(filter_args, sizeof(filter_args),
             "time_base=%d/"
             "%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
             time_base.num, time_base.den, avc_ctx->sample_rate,
             av_get_sample_fmt_name(avc_ctx->sample_fmt),
             avc_ctx->channel_layout);
    if (avfilter_graph_create_filter(&buffersrc_ctx,
                                     avfilter_get_by_name("abuffer"), "in",
                                     filter_args, NULL, filter_graph) < 0)
    {
        averror("could not create audio source buffer");
        goto error;
    }

    /* Output buffer */
    if (avfilter_graph_create_filter(&buffersink_ctx,
                                     avfilter_get_by_name("abuffersink"), "out",
                                     NULL, NULL, filter_graph) < 0)
    {
        averror("could not create audio sink buffer");
        goto error;
    }
    {
        const int64_t out_channel_layouts[] = {AV_CH_LAYOUT_MONO, -1};
        const int out_sample_rates[] = {DST_SAMPLE_RATE, -1};
        const enum AVSampleFormat out_sample_fmts[] = {AV_SAMPLE_FMT_U8, -1};
        if (av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts,
                                -1, AV_OPT_SEARCH_CHILDREN) < 0)
        {
            averror("could not set output sample format");
            goto error;
        }
        if (av_opt_set_int_list(buffersink_ctx, "channel_layouts",
                                out_channel_layouts, -1,
                                AV_OPT_SEARCH_CHILDREN) < 0)
        {
            averror("could not set output channel layout");
            goto error;
        }
        if (av_opt_set_int_list(buffersink_ctx, "sample_rates",
                                out_sample_rates, -1,
                                AV_OPT_SEARCH_CHILDREN) < 0)
        {
            averror("could not set output sample rate");
            goto error;
        }
    }

    /* Connect io */
    {
        AVFilterInOut *inputs = avfilter_inout_alloc();
        AVFilterInOut *outputs = avfilter_inout_alloc();
        if (!outputs || !inputs)
        {
            averror("could not allocate filter input/output");
            avfilter_inout_free(&inputs);
            avfilter_inout_free(&outputs);
            return 3;
        }
        inputs->name = av_strdup("out");
        inputs->filter_ctx = buffersink_ctx;
        inputs->pad_idx = 0;
        inputs->next = NULL;
        outputs->name = av_strdup("in");
        outputs->filter_ctx = buffersrc_ctx;
        outputs->pad_idx = 0;
        outputs->next = NULL;

        /* The main functions are specified in the filter specification */
        if (avfilter_graph_parse_ptr(filter_graph, FILTER, &inputs, &outputs,
                                     NULL) < 0)
            goto error;
        if (avfilter_graph_config(filter_graph, NULL) < 0)
            goto error;

        avfilter_inout_free(&inputs);
        avfilter_inout_free(&outputs);
    }

    *pbuffersrc_ctx = buffersrc_ctx;
    *pbuffersink_ctx = buffersink_ctx;
    return 0;

error:
    avfilter_graph_free(&filter_graph);
    return 3;
}

/* Return values:
 * - 0: success
 * - 1: argument error
 * - 2: user-supplied parameters are bad
 * - 3: other internal error
 */
int main(int argc, char **argv)
{
    AVStream *ast;
    AVFilterContext *buffersink_ctx;
    AVFilterContext *buffersrc_ctx;
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *avc_ctx = NULL;
    AVFilterGraph *filter_graph = NULL;
    AVPacket *packet = NULL;
    AVFrame *frame = NULL, *filtered_frame = NULL;
    int ret = 0;

    /* Initialization steps: parse arguments and init FFmpeg */
    parse_args(argc, argv);
    if (app_data.if_verbose)
        av_log_set_level(AV_LOG_VERBOSE);
    avformat_network_init();

    if ((ret = open_audio_stream(&fmt_ctx, &avc_ctx, &ast)) != 0)
        return ret;

    if ((ret = create_filters(ast, avc_ctx, &buffersrc_ctx, &buffersink_ctx)) !=
        0)
        goto error;

    if (!(packet = av_packet_alloc()))
    {
        averror("could not allocate packet");
        ret = 3;
        goto error;
    }
    if (!(frame = av_frame_alloc()))
    {
        averror("could not allocate frame");
        ret = 3;
        goto error;
    }
    if (!(filtered_frame = av_frame_alloc()))
    {
        averror("could not allocate filtered frame");
        ret = 3;
        goto error;
    }

    while (true)
    {
        int stat = 0;
        if (av_read_frame(fmt_ctx, packet) < 0)
            break;
        if (packet->stream_index == ast->index)
        {
            if (avcodec_send_packet(avc_ctx, packet) < 0)
            {
                averror("error while sending a packet to the decoder");
                ret = 3;
                goto error;
            }
            while (true)
            {
                stat = avcodec_receive_frame(avc_ctx, frame);
                if (stat == AVERROR(EAGAIN) || stat == AVERROR_EOF)
                    break;
                if (stat < 0)
                {
                    averror("error while receiving a frame from the decoder");
                    ret = 3;
                    goto error;
                }
                /* if (stat >= 0) */
                if (av_buffersrc_add_frame_flags(
                        buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                {
                    averror("error while feeding the audio filter");
                    break;
                }
                while (true)
                {
                    stat =
                        av_buffersink_get_frame(buffersink_ctx, filtered_frame);
                    if (stat == AVERROR(EAGAIN) || stat == AVERROR_EOF)
                        break;
                    if (stat < 0)
                    {
                        averror(
                            "error while receiving a frame from the filter");
                        ret = 3;
                        goto error;
                    }
                    {
                        /* Assuming mono 8-bit */
                        const int n = filtered_frame->nb_samples;
                        const uint8_t *p = filtered_frame->data[0];
                        const uint8_t *p_end = p + n;
                        while (p < p_end)
                            fputc(*p++, app_data.output);
                    }
                    av_frame_unref(filtered_frame);
                }
                av_frame_unref(frame);
                if (stat < 0)
                    break;
            }
        }
        av_packet_unref(packet);
    }

error:
    av_frame_free(&filtered_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&avc_ctx);
    avformat_close_input(&fmt_ctx);
    fclose(app_data.output);
    return ret;
}
