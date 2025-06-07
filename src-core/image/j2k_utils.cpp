#include "j2k_utils.h"
#include "libs/openjp2/openjpeg.h"
#include <cstring>

namespace satdump
{
    namespace image
    {
        Image decompress_j2k_openjp2(uint8_t *data, int length)
        {
            Image img;

            // Init decoder parameters
            opj_dparameters_t core;
            memset(&core, 0, sizeof(opj_dparameters_t));
            opj_set_default_decoder_parameters(&core);

            // Set input buffer info struct
            opj_buffer_info bufinfo;
            bufinfo.buf = data;
            bufinfo.cur = data;
            bufinfo.len = length;

            // Setup image, stream and codec
            opj_image_t *image = NULL;
            opj_stream_t *l_stream = opj_stream_create_buffer_stream(&bufinfo, true);
            opj_codec_t *l_codec = opj_create_decompress(OPJ_CODEC_J2K);

            // Check we could open the stream
            if (!l_stream)
            {
                opj_destroy_codec(l_codec);
                return img;
            }

            // Setup decoder
            if (!opj_setup_decoder(l_codec, &core))
            {
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                return img;
            }

            // Read header
            if (!opj_read_header(l_stream, l_codec, &image))
            {
                opj_stream_destroy(l_stream);
                opj_destroy_codec(l_codec);
                opj_image_destroy(image);
                return img;
            }

            // Decode image
            if (!(opj_decode(l_codec, l_stream, image) && opj_end_decompress(l_codec, l_stream)))
            {
                opj_destroy_codec(l_codec);
                opj_stream_destroy(l_stream);
                opj_image_destroy(image);
                return img;
            }

            // Parse into CImg
            img = Image(16, image->x1, image->y1, 1);
            for (int i = 0; i < int(image->x1 * image->y1); i++)
                img.set(i, image->comps[0].data[i]);

            // Free everything up
            opj_destroy_codec(l_codec);
            opj_stream_destroy(l_stream);
            opj_image_destroy(image);

            return img;
        }
    } // namespace image
} // namespace satdump