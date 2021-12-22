#include "jpeg_utils.h"
#include <stdexcept>
#include <cstring>
#include <csetjmp>
extern "C"
{
#include "libs/jpeg/jpeglib.h"
}
#include "libs/openjp2/openjpeg.h"

namespace image
{
    struct jpeg_error_struct
    {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    static void libjpeg_error_func(j_common_ptr cinfo)
    {
        longjmp(((jpeg_error_struct *)cinfo->err)->setjmp_buffer, 1);
    }

    static void libjpeg_error_func_ignore(j_common_ptr /*cinfo*/)
    {
        //longjmp(((jpeg_error_struct *)cinfo->err)->setjmp_buffer, 1);
    }

    Image<uint8_t> decompress_jpeg(uint8_t *data, int length, bool ignore_errors)
    {
        Image<uint8_t> img;
        unsigned char *jpeg_decomp = NULL;

        // Huge thanks to https://gist.github.com/PhirePhly/3080633
        jpeg_error_struct jerr;
        jpeg_decompress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = ignore_errors ? libjpeg_error_func_ignore : libjpeg_error_func;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp;
            return img;
        }

        jpeg_create_decompress(&cinfo);

        // Parse and start decompressing
        jpeg_mem__src(&cinfo, data, length);
        jpeg_read_header(&cinfo, FALSE);
        jpeg_start_decompress(&cinfo);

        // Init output buffer
        jpeg_decomp = new unsigned char[cinfo.image_width * cinfo.image_height];

        // Decompress
        while (cinfo.output_scanline < cinfo.output_height)
        {
            unsigned char *buffer_array[1];
            buffer_array[0] = jpeg_decomp + (cinfo.output_scanline) * cinfo.image_width;
            jpeg_read_scanlines(&cinfo, buffer_array, 1);
        }

        // Cleanup
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        // Init CImg image
        img = Image<uint8_t>(cinfo.image_width, cinfo.image_height, 1);

        // Copy over
        for (int i = 0; i < (int)cinfo.image_width * (int)cinfo.image_height; i++)
            img[i] = jpeg_decomp[i];

        // Free memory
        delete[] jpeg_decomp;

        return img;
    }

    Image<uint16_t> decompress_j2k_openjp2(uint8_t *data, int length)
    {
        Image<uint16_t> img;

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
        if (!(opj_decode(l_codec, l_stream, image) &&
              opj_end_decompress(l_codec, l_stream)))
        {
            opj_destroy_codec(l_codec);
            opj_stream_destroy(l_stream);
            opj_image_destroy(image);
            return img;
        }

        // Parse into CImg
        img = Image<uint16_t>(image->x1, image->y1, 1);
        for (int i = 0; i < int(image->x1 * image->y1); i++)
            img[i] = image->comps[0].data[i];

        // Free everything up
        opj_destroy_codec(l_codec);
        opj_stream_destroy(l_stream);
        opj_image_destroy(image);

        return img;
    }
}