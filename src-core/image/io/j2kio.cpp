#include "../io.h"
#include "libs/openjp2/openjpeg.h"
#include "logger.h"
#include <cstring>
#include <filesystem>

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace satdump
{
    namespace image
    {
        void save_j2k(Image &img, std::string file)
        {
            auto d_depth = img.depth();
            auto d_channels = img.channels();
            auto d_height = img.height();
            auto d_width = img.width();

            if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
            {
                logger->trace("Tried to save empty J2K!");
                return;
            }

            opj_cparameters_t parameters;
            opj_image_t *image;
            OPJ_CODEC_FORMAT codec_format = OPJ_CODEC_J2K;

            opj_set_default_encoder_parameters(&parameters);
            parameters.numresolution = 5;

            {
                opj_image_cmptparm_t comp_params[4];
                memset(comp_params, 0, 4 * sizeof(opj_image_cmptparm_t));

                for (int i = 0; i < d_channels; i++)
                {
                    comp_params[i].prec = d_depth;
                    // comp_params[i].bpp = 8;
                    comp_params[i].sgnd = 0;
                    comp_params[i].dx = parameters.subsampling_dx;
                    comp_params[i].dy = parameters.subsampling_dy;
                    comp_params[i].w = d_width;
                    comp_params[i].h = d_height;
                }

                image = opj_image_create(d_channels, comp_params, d_channels == 1 ? OPJ_CLRSPC_GRAY : OPJ_CLRSPC_SRGB);

                if (image == NULL)
                {
                    logger->error("JP2 image is null? C");
                }
                else
                {
                    image->x0 = 0;
                    image->y0 = 0;
                    image->x1 = (d_width - 1) * parameters.subsampling_dx + 1;
                    image->y1 = (d_height - 1) * parameters.subsampling_dy + 1;

                    // Put into image
                    for (int c = 0; c < d_channels; c++)
                        for (size_t i = 0; i < d_width * d_height; i++)
                            image->comps[c].data[i] = img.get(c, i);
                }
            }

            if (image == NULL)
            {
                logger->error("JP2 image is null? F");
                return;
            }

            opj_stream_t *stream;
            opj_codec_t *codec;

            stream = NULL;
            parameters.tcp_mct = image->numcomps == 1 ? 0 : 1;

            codec = opj_create_compress(codec_format);

            if (codec == NULL)
                goto abort;

            opj_setup_encoder(codec, &parameters, image);

            stream = opj_stream_create_default_file_stream(file.c_str(), false);

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

        abort:

            if (stream)
                opj_stream_destroy(stream);

            if (codec)
                opj_destroy_codec(codec);

            opj_image_destroy(image);

            if (parameters.cp_comment)
                free(parameters.cp_comment);

            return;
        }

        void load_j2k(Image &img, std::string file)
        {
            if (!std::filesystem::exists(file))
                return;

            // Init decoder parameters
            opj_dparameters_t core;
            memset(&core, 0, sizeof(opj_dparameters_t));
            opj_set_default_decoder_parameters(&core);

            // Setup image, stream and codec
            opj_image_t *image = NULL;
            opj_stream_t *l_stream = opj_stream_create_file_stream(file.c_str(), OPJ_J2K_STREAM_CHUNK_SIZE, true);
            opj_codec_t *l_codec = opj_create_decompress(OPJ_CODEC_J2K);

            // Check we could open the stream
            if (!l_stream)
            {
                opj_destroy_codec(l_codec);
                return;
            }

            // Setup decoder
            if (!opj_setup_decoder(l_codec, &core))
            {
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                return;
            }

            // Read header
            if (!opj_read_header(l_stream, l_codec, &image))
            {
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                opj_image_destroy(image);
                return;
            }

            // Decode image
            if (!(opj_decode(l_codec, l_stream, image) && opj_end_decompress(l_codec, l_stream)))
            {
                opj_destroy_codec(l_codec);
                opj_stream_destroy(l_stream);
                opj_image_destroy(image);
                return;
            }

            // Parse into image
            int depth = image->comps[0].prec;
            int d_depth = 8;
            if (depth > 8)
                d_depth = 16;
            img.init(d_depth, image->x1, image->y1, image->numcomps);

            if (depth > 8)
            {
                for (uint32_t c = 0; c < image->numcomps; c++)
                    for (int i = 0; i < int(image->x1 * image->y1); i++)
                        img.set(c, i, image->comps[c].data[i] << (16 - depth));
            }
            else
            {
                for (uint32_t c = 0; c < image->numcomps; c++)
                    for (int i = 0; i < int(image->x1 * image->y1); i++)
                        img.set(c, i, image->comps[c].data[i] << (8 - depth));
            }

            // Free everything up
            opj_destroy_codec(l_codec);
            opj_stream_destroy(l_stream);
            opj_image_destroy(image);
        }
    } // namespace image
} // namespace satdump