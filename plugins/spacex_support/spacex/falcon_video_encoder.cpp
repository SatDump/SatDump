/*
 * This file is part of the SatDump
 *
 * Autors:
 *   Oleg Kutkov <contact@olegkutkov.me>
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef USE_VIDEO_ENCODER

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <stdio.h>
}

#include <algorithm>
#include <stdexcept>

#include "falcon_video_encoder.hpp"
#include "logger.h"

///

#define TS_SYNC_BYTE 0x47
#define TS_DATA_START 0x40
#define AVIO_BUF_SIZE 204800

static const std::string overlay_file = "telemetry_overlat.txt";
static const std::string overlay_params = ":reload=1:x=15:y=310:fontsize=15:fontcolor=white";

///

namespace spacex
{
    FalconVideoEncoder::FalconVideoEncoder(std::string out_dir)
        : io_ctx(NULL), in_ctx(NULL), filter_ctx(NULL), avio_buf_len(AVIO_BUF_SIZE), avio_buf(static_cast<uint8_t *>(av_malloc(AVIO_BUF_SIZE))), buffersrc(avfilter_get_by_name("buffer")), buffersink(avfilter_get_by_name("buffersink")), out_directory(out_dir), overlay_txt_file(out_dir + "/" + overlay_file), filter_desc("drawtext=textfile='" + overlay_txt_file + "'" + overlay_params), data_found(false), decoder_started(false)
    {
        if (!avio_buf)
        {
            throw std::runtime_error("Failed to allocate avio buffer\n");
        }

        stream_buffer.reserve(AVIO_BUF_SIZE);

        io_ctx = avio_alloc_context(avio_buf, avio_buf_len, 0, &stream_buffer, &FalconVideoEncoder::avReader, NULL, NULL);

        if (!io_ctx)
        {
            throw std::runtime_error("Failed to allocate avio context\n");
        }

        io_ctx->max_packet_size = avio_buf_len;
        in_ctx = avformat_alloc_context();
        in_ctx->pb = io_ctx;

        memset(avio_buf, 0, AVIO_BUF_SIZE);

        av_log_set_callback(FalconVideoEncoder::libraryLogger);

        overlay.open(overlay_txt_file, std::ios::out | std::ios::trunc);

        if (!overlay.is_open())
        {
            throw std::runtime_error("Failed to open overlay text file\n");
        }
    }

    FalconVideoEncoder::~FalconVideoEncoder()
    {
        if (in_ctx)
        {
            avformat_close_input(&in_ctx);
            avformat_free_context(in_ctx);
        }

        if (io_ctx)
        {
            av_free(io_ctx);
        }

        overlay.close();
        remove(overlay_txt_file.c_str());

        for (std::pair<uint32_t, AVCodecContext *> dec : video_decoders)
        {
            avcodec_free_context(&dec.second);
        }

        for (std::pair<uint32_t, AVCodecContext *> enc : video_encoders)
        {
            avcodec_free_context(&enc.second);
        }

        for (std::pair<uint32_t, VideoFilter> filter : video_filters)
        {
            avfilter_graph_free(&filter.second.filter_graph);
        }
    }

    void FalconVideoEncoder::pushTelemetryText(std::string text)
    {
        overlay.seekp(0);
        overlay << text << std::endl;
    }

    void FalconVideoEncoder::libraryLogger(void *ptr, int level, const char *fmt, va_list vl)
    {
        (void)ptr;
        char str[1024] = {0};

        vsnprintf(str, sizeof(str), fmt, vl);
        str[strlen(str) - 1] = '\0';

        switch (level)
        {
        case AV_LOG_PANIC:
        case AV_LOG_FATAL:
            logger->critical(str);
            break;

        case AV_LOG_ERROR:
            logger->error(str);
            break;

        case AV_LOG_INFO:
            logger->info(str);
            break;

        case AV_LOG_WARNING:
            logger->warn(str);
            break;

        case AV_LOG_DEBUG:
            logger->debug(str);
            break;

        default:
            logger->trace(std::string("FalconVideoEncoder: ") + str);
        };
    }

    bool FalconVideoEncoder::feedData(std::vector<uint8_t>::const_iterator &data_chunk_start,
                                      std::vector<uint8_t>::const_iterator &data_chunk_end)
    {
        if ((stream_buffer.size() + std::distance(data_chunk_start, data_chunk_end)) < AVIO_BUF_SIZE)
        {
            stream_buffer.insert(stream_buffer.begin() + stream_buffer.size(), data_chunk_start, data_chunk_end);
            return true;
        }

        if (!decoder_started)
        {
            decoder_started = initDecoder();

            if (!decoder_started)
            {
                stream_buffer.clear();
                return false;
            }
        }

        frameWorker();

        stream_buffer.clear();

        return true;
    }

    bool FalconVideoEncoder::streamFinished()
    {
        if (!decoder_started && stream_buffer.size())
        {
            decoder_started = initDecoder();

            if (decoder_started)
            {
                frameWorker();
                stream_buffer.clear();
            }
        }

        if (decoder_started)
        {
            for (std::pair<uint32_t, AVFormatContext *> out_fmt_ctx : video_out_ctx)
            {
                av_write_trailer(out_fmt_ctx.second);
                avio_closep(&(out_fmt_ctx.second->pb));
                avformat_free_context(out_fmt_ctx.second);
            }
        }

        return true;
    }
    ////

    int FalconVideoEncoder::avReader(void *opaque, uint8_t *buf, int buf_size)
    {
        std::vector<uint8_t> *sb = (std::vector<uint8_t> *)opaque;

        buf_size = sb->size();

        if (sb->size())
        {
            std::copy(sb->begin(), sb->end(), buf);
            sb->clear();
        }

        return buf_size;
    }

    bool FalconVideoEncoder::initDecoder()
    {
        int ret = avformat_open_input(&in_ctx, "VIDEO", NULL, NULL);

        if (ret < 0)
        {
            logger->error("avformat_open_input() failed!");
            return false;
        }

        if (avformat_find_stream_info(in_ctx, NULL) < 0)
        {
            logger->error("Failed to find stream info");
            return false;
        }

        for (uint32_t i = 0; i < in_ctx->nb_streams; i++)
        {
            AVStream *stream = in_ctx->streams[i];

            if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);

                if (!dec)
                {
                    logger->error("Couldn't find decoder for stream #" + std::to_string(i));
                    return false;
                }

                AVCodecContext *codec_ctx = avcodec_alloc_context3(dec);

                if (!codec_ctx)
                {
                    logger->error("AVCODEC context allocation failed");
                    return false;
                }

                if (avcodec_parameters_to_context(codec_ctx, stream->codecpar) < 0)
                {
                    logger->error("Failed to copy decoder parameters to input decoder context");
                    return false;
                }

                logger->info("Found Video stream, id = " + std::to_string(i));

                codec_ctx->framerate = av_guess_frame_rate(in_ctx, stream, NULL);

                if (avcodec_open2(codec_ctx, avcodec_find_decoder(codec_ctx->codec_id), NULL) < 0)
                {
                    logger->error("Can't open decoder for the stream");
                    return false;
                }

                video_decoders[i] = codec_ctx;
            }
            else if (stream->codecpar->codec_type == AVMEDIA_TYPE_DATA)
            {
                if (stream->codecpar->codec_id == AV_CODEC_ID_SMPTE_KLV)
                {
                    logger->info("Found KLV data stream, id = " + std::to_string(i));
                    klv_streams[i].open(buildOutFileName(i, "data", "klv").c_str());
                }
            }
        }

        av_dump_format(in_ctx, 0, "STREAM", 0);

        if (!configureVideoOutput())
        {
            return false;
        }

        if (!configureVideoFilters())
        {
            return false;
        }

        return true;
    }

    std::string FalconVideoEncoder::buildOutFileName(uint32_t stream_id, const char *base, const char *ext)
    {
        std::string result = out_directory + "/" + base + "_" + std::to_string(stream_id) + "." + ext;
        return result;
    }

    bool FalconVideoEncoder::configureVideoOutput()
    {
        AVStream *out_stream;
        AVFormatContext *out_ctx;
        AVCodecContext *enc_ctx;
        AVCodec *encoder;

        for (std::pair<uint32_t, AVCodecContext *> dec_ctx : video_decoders)
        {
            std::string out_file = buildOutFileName(dec_ctx.first, "video", "mp4");

            avformat_alloc_output_context2(&out_ctx, NULL, NULL, out_file.c_str());

            if (!out_ctx)
            {
                logger->error("Failed to create output context file " + out_file);
                continue;
            }

            encoder = avcodec_find_encoder(dec_ctx.second->codec_id);

            if (!encoder)
            {
                logger->error("Failed to find encoder for stream #" + std::to_string(dec_ctx.first));
                continue;
            }

            out_stream = avformat_new_stream(out_ctx, encoder);

            if (!out_stream)
            {
                logger->error("Failed allocating output stream " + out_file);
                continue;
            }

            enc_ctx = avcodec_alloc_context3(encoder);

            if (!enc_ctx)
            {
                logger->error("Failed to allocate the encoder context for " + out_file);
                continue;
            }

            enc_ctx->width = dec_ctx.second->width;
            enc_ctx->height = dec_ctx.second->height;
            enc_ctx->sample_aspect_ratio = dec_ctx.second->sample_aspect_ratio;

            if (encoder->pix_fmts)
            {
                enc_ctx->pix_fmt = encoder->pix_fmts[0];
            }
            else
            {
                enc_ctx->pix_fmt = dec_ctx.second->pix_fmt;
            }

            enc_ctx->time_base = av_inv_q(dec_ctx.second->framerate);

            if (out_ctx->oformat->flags & AVFMT_GLOBALHEADER)
            {
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }

            if (avcodec_open2(enc_ctx, encoder, NULL) < 0)
            {
                logger->error("Couldn't open video encoder for stream #" + std::to_string(dec_ctx.first));
                continue;
            }

            avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);

            out_stream->time_base = enc_ctx->time_base;

            if (avio_open(&out_ctx->pb, out_file.c_str(), AVIO_FLAG_WRITE) < 0)
            {
                logger->error("Couldn't create output file " + out_file);
                continue;
            }

            if (avformat_write_header(out_ctx, NULL) < 0)
            {
                logger->error("Unable to write AV header to " + out_file);
                continue;
            }

            logger->info("Writing Video #" + std::to_string(dec_ctx.first) + " to " + out_file);

            video_encoders[dec_ctx.first] = enc_ctx;
            video_out_ctx[dec_ctx.first] = out_ctx;

            av_dump_format(out_ctx, 0, out_file.c_str(), 1);
        }

        return true;
    }

    bool FalconVideoEncoder::configureVideoFilters()
    {
        char args[512];

        for (std::pair<uint32_t, AVCodecContext *> dec_ctx : video_decoders)
        {
            AVFilterGraph *fgraph = avfilter_graph_alloc();

            if (!fgraph)
            {
                logger->error("Failed to allocate filter graph");
                return false;
            }

            AVStream *stream = in_ctx->streams[dec_ctx.first];
            AVFilterInOut *outputs = avfilter_inout_alloc();
            AVFilterInOut *inputs = avfilter_inout_alloc();

            if (!outputs || !inputs)
            {
                logger->error("Failed to allocate filter inputs and outputs");
                return false;
            }

            AVRational time_base = stream->time_base;

            snprintf(args, sizeof(args),
                     "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                     dec_ctx.second->width, dec_ctx.second->height, dec_ctx.second->pix_fmt,
                     time_base.num, time_base.den,
                     dec_ctx.second->sample_aspect_ratio.num, dec_ctx.second->sample_aspect_ratio.den);

            AVFilterContext *buffersrc_ctx;
            AVFilterContext *buffersink_ctx;

            if (avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in", args, NULL, fgraph) < 0)
            {
                logger->error("Cannot create filter buffer source");
                return false;
            }

            if (avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out", NULL, NULL, fgraph) < 0)
            {
                logger->error("Cannot create filter buffer sink");
                return false;
            }

            outputs->name = av_strdup("in");
            outputs->filter_ctx = buffersrc_ctx;
            outputs->pad_idx = 0;
            outputs->next = NULL;

            inputs->name = av_strdup("out");
            inputs->filter_ctx = buffersink_ctx;
            inputs->pad_idx = 0;
            inputs->next = NULL;

            if (avfilter_graph_parse_ptr(fgraph, filter_desc.c_str(), &inputs, &outputs, NULL) < 0)
            {
                logger->error("avfilter_graph_parse_ptr fail");
                return false;
            }

            if (avfilter_graph_config(fgraph, NULL) < 0)
            {
                logger->error("avfilter_graph_config failed");
                return false;
            }

            video_filters[dec_ctx.first] = {buffersrc_ctx, buffersink_ctx, fgraph};
        }

        return true;
    }

    bool FalconVideoEncoder::frameDecoder(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt)
    {
        int ret;
        *got_frame = 0;

        if (pkt)
        {
            ret = avcodec_send_packet(avctx, pkt);

            if (ret < 0 && ret != AVERROR_EOF)
            {
                return false;
            }
        }

        ret = avcodec_receive_frame(avctx, frame);

        if (ret < 0 && ret != AVERROR(EAGAIN))
        {
            return false;
        }

        if (ret >= 0)
        {
            *got_frame = 1;
        }

        return true;
    }

    bool FalconVideoEncoder::frameWorker()
    {
        AVPacket packet;
        AVPacket enc_pkt;

        std::unordered_map<uint32_t, AVCodecContext *>::iterator dec_it;
        std::unordered_map<uint32_t, AVCodecContext *>::iterator enc_it;
        std::unordered_map<uint32_t, AVFormatContext *>::iterator out_ctx_it;
        std::unordered_map<uint32_t, VideoFilter>::iterator vfilter_it;
        std::unordered_map<uint32_t, std::ofstream>::iterator klv_it;

        AVCodecContext *dec;
        AVCodecContext *enc;
        AVFormatContext *out_ctx;

        int got_frame = 0, ret = 0;

        AVFrame *frame = av_frame_alloc();
        AVFrame *filt_frame = av_frame_alloc();

        if (!frame || !filt_frame)
        {
            logger->error("Couldn't allocate frame");
            return false;
        }

        av_init_packet(&packet);

        while (av_read_frame(in_ctx, &packet) >= 0)
        {
            uint32_t stream_id = packet.stream_index;
            dec_it = video_decoders.find(stream_id);

            /* Found decoder for the stream */
            if (dec_it != video_decoders.end())
            {

                enc_it = video_encoders.find(stream_id);

                if (enc_it == video_encoders.end())
                {
                    logger->error("Can't find video encoder for stream #" + std::to_string(stream_id));
                    return false;
                }

                out_ctx_it = video_out_ctx.find(stream_id);

                if (out_ctx_it == video_out_ctx.end())
                {
                    logger->error("Can't find output context for stream #" + std::to_string(stream_id));
                    return false;
                }

                vfilter_it = video_filters.find(stream_id);

                if (vfilter_it == video_filters.end())
                {
                    logger->error("Can't find video filter for stream #" + std::to_string(stream_id));
                    return false;
                }

                dec = dec_it->second;
                enc = enc_it->second;
                out_ctx = out_ctx_it->second;

                av_packet_rescale_ts(&packet, in_ctx->streams[stream_id]->time_base, dec->time_base);

                if (!frameDecoder(dec, frame, &got_frame, &packet))
                {
                    logger->error("Stream #" + std::to_string(stream_id) + " decoder failed");
                    return false;
                }

                if (frame_counters.find(stream_id) == frame_counters.end())
                {
                    frame_counters[stream_id] = 0;
                }

                frame->pts = frame_counters[stream_id]++;

                if (got_frame)
                {

                    if (av_buffersrc_add_frame_flags(vfilter_it->second.buf_src_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
                    {
                        logger->error("Error while feeding the filtergraph");
                        continue;
                    }

                    while (1)
                    {
                        ret = av_buffersink_get_frame(vfilter_it->second.buf_sink_ctx, filt_frame);

                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        {
                            break;
                        }
                        else if (ret < 0)
                        {
                            return -1;
                        }

                        if (avcodec_send_frame(enc, filt_frame) < 0)
                        {
                            break;
                        }

                        av_init_packet(&enc_pkt);

                        while (ret >= 0)
                        {
                            ret = avcodec_receive_packet(enc, &enc_pkt);

                            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            {
                                break;
                            }
                            else if (ret < 0)
                            {
                                logger->error("Encoder failure! Stream #" + std::to_string(stream_id));
                                return false;
                            }

                            enc_pkt.stream_index = 0;
                            av_packet_rescale_ts(&enc_pkt, enc->time_base, out_ctx->streams[0]->time_base);

                            av_interleaved_write_frame(out_ctx, &enc_pkt);
                        } /* while */
                    }     /* while(1) */
                }         /* got_frame */
            }
            else
            { /* Data */
                klv_it = klv_streams.find(stream_id);

                if (klv_it != klv_streams.end())
                {
                    klv_it->second.write(reinterpret_cast<const char *>(packet.data), packet.size);
                }
            }

            av_packet_unref(&packet);
        }

        av_frame_unref(frame);
        av_frame_unref(filt_frame);

        return true;
    }

} // namespace spacex

#endif // USE_VIDEO_ENCODER
