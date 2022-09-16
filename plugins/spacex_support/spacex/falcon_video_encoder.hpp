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

#ifndef FALCON_VIDEO_ENCODER_HPP
#define FALCON_VIDEO_ENCODER_HPP

#ifdef USE_VIDEO_ENCODER

#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>

class AVIOContext;
class AVFormatContext;
class AVCodecContext;
class AVFilterContext;
class AVFilterGraph;
class AVFilter;
class AVPacket;
class AVFrame;

namespace spacex
{
    class FalconVideoEncoder
    {
    public:
        FalconVideoEncoder(std::string out_dir);
        ~FalconVideoEncoder();

        bool feedData(std::vector<uint8_t>::const_iterator &data_chunk_start,
                      std::vector<uint8_t>::const_iterator &data_chunk_end);
        bool processData();
        bool streamFinished();

        void pushTelemetryText(std::string text);

    private:
        bool initDecoder();

        static void libraryLogger(void *ptr, int level, const char *fmt, va_list vl);

        static int avReader(void *opaque, uint8_t *buf, int buf_size);

        bool configureVideoOutput();
        bool configureVideoFilters();

        bool frameDecoder(AVCodecContext *avctx, AVFrame *frame, int *got_frame, AVPacket *pkt);
        bool frameWorker();

        std::string buildOutFileName(uint32_t stream_id, const char *base, const char *ext);

        ////
        struct VideoFilter
        {
            AVFilterContext *buf_src_ctx;
            AVFilterContext *buf_sink_ctx;
            AVFilterGraph *filter_graph;
        };

        AVIOContext *io_ctx;
        AVFormatContext *in_ctx;
        AVFilterContext *filter_ctx;

        size_t avio_buf_len;
        uint8_t *avio_buf;

        const AVFilter *buffersrc;
        const AVFilter *buffersink;

        std::string out_directory;
        std::string overlay_txt_file;
        std::string filter_desc;

        bool data_found;
        bool decoder_started;

        std::vector<uint8_t> stream_buffer;

        /// key - stream ID
        std::unordered_map<uint32_t, AVCodecContext *> video_decoders;
        std::unordered_map<uint32_t, AVCodecContext *> video_encoders;
        std::unordered_map<uint32_t, AVFormatContext *> video_out_ctx;
        std::unordered_map<uint32_t, VideoFilter> video_filters;
        std::unordered_map<uint32_t, std::ofstream> klv_streams;

        std::unordered_map<uint32_t, uint32_t> frame_counters;

        std::ofstream overlay;
    };
}

#endif // USE_VIDEO_ENCODER
#endif // FALCON_VIDEO_ENCODER_HPP
