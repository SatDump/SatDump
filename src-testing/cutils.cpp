#include "cutils.h"
#include "libs/openjp2/openjpeg.h"
#include "logger.h"
#include <cstring>

void calculate_chunking_stategy(int dim_size,
                                int default_num_chunks,
                                int min_chunk_size,
                                int *num_chunks,
                                int *chunk_size)
{
    *num_chunks = default_num_chunks;
    *chunk_size = default_num_chunks;
    while ((dim_size / *num_chunks) < min_chunk_size &&
           *num_chunks > 1)
        *num_chunks -= 1;
    if (*num_chunks == 1)
        *chunk_size = dim_size;
    else if (*chunk_size * *num_chunks != dim_size)
        *chunk_size = (dim_size / *num_chunks) + 1;
    else
        *chunk_size = dim_size / *num_chunks;
}

std::vector<ccsds::CCSDSPacket> encode_image_chunked(image::Image<uint8_t> source_image, time_t timestamp, int apid, int channel, int maxsize)
{
    std::vector<ccsds::CCSDSPacket> encoded_pkts;

    int num_chunks_x = 0;
    int num_chunks_y = 0;
    int chunk_size_x = 0;
    int chunk_size_y = 0;

    calculate_chunking_stategy(source_image.width(), 16, 128, &num_chunks_x, &chunk_size_x);
    calculate_chunking_stategy(source_image.height(), 16, 128, &num_chunks_y, &chunk_size_y);

    logger->trace("X %d %d", num_chunks_x, chunk_size_x);
    logger->trace("Y %d %d", num_chunks_y, chunk_size_y);

    int seq_cnt = 0;
    for (int x = 0; x < num_chunks_x; x++)
    {
        for (int y = 0; y < num_chunks_y; y++)
        {
            int x_s = chunk_size_x * x;
            int y_s = chunk_size_y * y;
            int x_e = chunk_size_x * (x + 1);
            int y_e = chunk_size_y * (y + 1);

            if (x_e >= source_image.width())
                x_e = source_image.width() - 1;
            if (y_e >= source_image.height())
                y_e = source_image.height() - 1;

            image::Image<uint16_t> img_crop = source_image.crop_to(x_s, y_s, x_e, y_e).to16bits();
            // img_crop.save_jpeg("SPLIT_IMAGE/" + std::to_string(x) + "_" + std::to_string(y) + ".jpg");

            for (int i = 0; i < img_crop.size(); i++)
                img_crop[i] >>= 8;

            std::vector<uint8_t> output;
            float quality = 90;
            float d_min_quality = 1;
            int d_max_seg_size = maxsize; // 1e3 7e3;

        re_compress:
            output = compress_openjpeg(img_crop.data(), img_crop.width(), img_crop.height(), 8, quality);

            if (output.size() > d_max_seg_size && quality >= d_min_quality)
            {
                quality -= 2;
                output.clear();
                goto re_compress;
            }

            std::ofstream("SPLIT_IMAGE/" + std::to_string(x) + "_" + std::to_string(y) + ".j2k", std::ios::binary).write((char *)output.data(), output.size());

            ccsds::CCSDSPacket pkt;
            pkt.header.version = 0;
            pkt.header.type = 0;
            pkt.header.secondary_header_flag = true;
            pkt.header.apid = apid;
            pkt.header.sequence_flag = 0b11;
            pkt.header.packet_sequence_count = seq_cnt;
            seq_cnt++;
            seq_cnt = seq_cnt % 16384;

            pkt.payload.resize(17);
            pkt.payload[0] = (timestamp >> 56) & 0xFF; // Timestamp
            pkt.payload[1] = (timestamp >> 48) & 0xFF;
            pkt.payload[2] = (timestamp >> 40) & 0xFF;
            pkt.payload[3] = (timestamp >> 32) & 0xFF;
            pkt.payload[4] = (timestamp >> 24) & 0xFF;
            pkt.payload[5] = (timestamp >> 16) & 0xFF;
            pkt.payload[6] = (timestamp >> 8) & 0xFF;
            pkt.payload[7] = (timestamp >> 0) & 0xFF;
            pkt.payload[8] = channel; // Channel

            pkt.payload[9] = source_image.width() >> 8;
            pkt.payload[10] = source_image.width() & 0xFF;

            pkt.payload[11] = source_image.height() >> 8;
            pkt.payload[12] = source_image.height() & 0xFF;

            pkt.payload[13] = x_s >> 8;
            pkt.payload[14] = x_s & 0xFF;
            pkt.payload[15] = y_s >> 8;
            pkt.payload[16] = y_s & 0xFF;

            pkt.payload.insert(pkt.payload.end(), output.begin(), output.end());
            pkt.encodeHDR();
            encoded_pkts.push_back(pkt);
        }
    }

    ccsds::CCSDSPacket pkt;
    pkt.header.version = 0;
    pkt.header.type = 0;
    pkt.header.secondary_header_flag = true;
    pkt.header.apid = apid;
    pkt.header.sequence_flag = 0b11;
    pkt.header.packet_sequence_count = seq_cnt;
    seq_cnt++;
    seq_cnt = seq_cnt % 16384;

    pkt.payload.resize(10);
    pkt.payload[0] = (timestamp >> 56) & 0xFF; // Timestamp
    pkt.payload[1] = (timestamp >> 48) & 0xFF;
    pkt.payload[2] = (timestamp >> 40) & 0xFF;
    pkt.payload[3] = (timestamp >> 32) & 0xFF;
    pkt.payload[4] = (timestamp >> 24) & 0xFF;
    pkt.payload[5] = (timestamp >> 16) & 0xFF;
    pkt.payload[6] = (timestamp >> 8) & 0xFF;
    pkt.payload[7] = (timestamp >> 0) & 0xFF;
    pkt.payload[8] = channel; // Channel
    pkt.payload[9] = 0xFF;

    pkt.encodeHDR();
    encoded_pkts.push_back(pkt);

    return encoded_pkts;
}

namespace
{
    struct OPJMemBuf
    {
        int current_pos = 0;
        std::vector<uint8_t> buf;
    };

    static OPJ_SIZE_T opj_write_from_file(void *p_buffer, OPJ_SIZE_T p_nb_bytes,
                                          void *p_user_data)
    {
        OPJMemBuf &res = *((OPJMemBuf *)p_user_data);
        // logger->trace("WRITE");
        if (res.current_pos + 1 + p_nb_bytes > res.buf.size())
            res.buf.resize(res.current_pos + 1 + p_nb_bytes);
        memcpy(&res.buf[res.current_pos], p_buffer, p_nb_bytes);
        res.current_pos += p_nb_bytes;
        return p_nb_bytes;
    }

    static OPJ_SIZE_T opj_read_from_file(void *p_buffer, OPJ_SIZE_T p_nb_bytes,
                                         void *p_user_data)
    {
        OPJMemBuf &res = *((OPJMemBuf *)p_user_data);
        // logger->trace("READ");
        //  READ
        return -1;
    }

    static OPJ_UINT64 opj_get_data_length_from_file(void *p_user_data)
    {
        OPJMemBuf &res = *((OPJMemBuf *)p_user_data);
        // logger->trace("GETSIZE");
        return (OPJ_UINT64)res.buf.size();
    }

    static void opj_close_from_file(void *p_user_data)
    {
        OPJMemBuf &res = *((OPJMemBuf *)p_user_data);
        // logger->trace("CLOSE");
    }

    static OPJ_OFF_T opj_skip_from_file(OPJ_OFF_T p_nb_bytes, void *p_user_data)
    {
        OPJMemBuf &res = *((OPJMemBuf *)p_user_data);
        //  logger->trace("SKIP");
        if (res.current_pos + 1 + p_nb_bytes >= res.buf.size())
            res.buf.resize(res.current_pos + 1 + p_nb_bytes);
        if (res.current_pos + p_nb_bytes < res.buf.size())
        {
            res.current_pos += p_nb_bytes;
            return p_nb_bytes;
        }
        else
        {
            return -1;
        }
    }

    static OPJ_BOOL opj_seek_from_file(OPJ_OFF_T p_nb_bytes, void *p_user_data)
    {
        OPJMemBuf &res = *((OPJMemBuf *)p_user_data);
        // logger->trace("SEEK");
        if (p_nb_bytes < res.buf.size())
        {
            res.current_pos = p_nb_bytes;
            return OPJ_TRUE;
        }
        else
        {
            return OPJ_FALSE;
        }
    }
}

std::vector<uint8_t> compress_openjpeg(uint16_t *img, int width, int height, int depth, float quality)
{
    std::vector<uint8_t> res;

    opj_cparameters_t parameters;
    opj_image_t *image;
    OPJ_CODEC_FORMAT codec_format = OPJ_CODEC_J2K;

    opj_set_default_encoder_parameters(&parameters);

    {
        int num_ch = 1;
        double d = 115.0 - (double)quality;
        double rate = 100.0 / (d * d);
        double header_size = 550.0;
        header_size += ((double)num_ch - 1.0) * 142.0;
        double current_size = (double)((height * width *
                                        depth) /
                                       8.0) *
                              (double)num_ch;
        double target_size = (current_size * rate) + header_size;
        rate = target_size / current_size;

        parameters.tcp_rates[0] = 1.0f / (float)rate;
        parameters.tcp_numlayers = 1;
        parameters.cp_disto_alloc = 1;
    }

    {
        opj_image_cmptparm_t cmptparm;
        cmptparm.prec = depth;
        cmptparm.bpp = 8;
        cmptparm.sgnd = 0;
        cmptparm.dx = parameters.subsampling_dx;
        cmptparm.dy = parameters.subsampling_dy;
        cmptparm.w = width;
        cmptparm.h = height;

        image = opj_image_create(1, &cmptparm, OPJ_CLRSPC_GRAY);

        if (image == NULL)
        {
            logger->error("JP2 image is null? C");
        }
        else
        {
            image->x0 = 0;
            image->y0 = 0;
            image->x1 = (width - 1) * parameters.subsampling_dx + 1;
            image->y1 = (height - 1) * parameters.subsampling_dy + 1;

            auto img_buf = image->comps[0].data;

            for (size_t i = 0; i < width * height; i++)
                img_buf[i] = img[i];
        }
    }

    if (image == NULL)
    {
        logger->error("JP2 image is null? F");
        return res;
    }

    opj_stream_t *stream;
    opj_codec_t *codec;
    OPJMemBuf buf;

    stream = NULL;
    parameters.tcp_mct = image->numcomps == 3 ? 1 : 0;

    codec = opj_create_compress(codec_format);

    if (codec == NULL)
        goto abort;

    opj_setup_encoder(codec, &parameters, image);

    // stream = opj_stream_create_default_file_stream("file_idk.j2k", false);
    {
        stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, false);
        if (!stream)
        {
            logger->info("NO STREAM!");
        }
        else
        {
            opj_stream_set_user_data(stream, &buf, opj_close_from_file);
            opj_stream_set_user_data_length(stream, opj_get_data_length_from_file(&buf));
            opj_stream_set_read_function(stream, opj_read_from_file);
            opj_stream_set_write_function(stream, (opj_stream_write_fn)opj_write_from_file);
            opj_stream_set_skip_function(stream, opj_skip_from_file);
            opj_stream_set_seek_function(stream, opj_seek_from_file);
        }
    }

    if (stream == NULL)
    {
        logger->error("NULL STREAM");
        goto abort;
    }

    if (!opj_start_compress(codec, image, stream))
    {
        logger->error("START COMPRESS ERROR");
        goto abort;
    }

    if (!opj_encode(codec, stream))
    {
        logger->error("ENCODE ERROR");
        goto abort;
    }

    opj_end_compress(codec, stream);

    res = buf.buf;

abort:

    if (stream)
        opj_stream_destroy(stream);

    if (codec)
        opj_destroy_codec(codec);

    opj_image_destroy(image);

    if (parameters.cp_comment)
        free(parameters.cp_comment);

    return res;
}