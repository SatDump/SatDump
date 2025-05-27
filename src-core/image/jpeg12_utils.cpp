#include "jpeg12_utils.h"
#include <csetjmp>
#include <cstring>
#include <stdexcept>
extern "C"
{
#include "libs/jpeg12/jpeglib12.h"
}
// #include "libs/openjp2/openjpeg.h"

namespace satdump
{
    namespace image
    {
        struct jpeg_error_struct12
        {
            struct jpeg_error_mgr pub;
            jmp_buf setjmp_buffer;
        };

        static void libjpeg_error_func12(j_common_ptr cinfo) { longjmp(((jpeg_error_struct12 *)cinfo->err)->setjmp_buffer, 1); }

        static void libjpeg_error_func_ignore12(j_common_ptr /*cinfo*/)
        {
            // longjmp(((jpeg_error_struct *)cinfo->err)->setjmp_buffer, 1);
        }

        Image decompress_jpeg12(uint8_t *data, int length, bool ignore_errors)
        {
            Image img;
            short *jpeg_decomp = NULL;

            // Huge thanks to https://gist.github.com/PhirePhly/3080633
            jpeg_error_struct12 jerr;
            jpeg_decompress_struct cinfo;

            // Init
            cinfo.err = jpeg_std_error(&jerr.pub);
            jerr.pub.error_exit = ignore_errors ? libjpeg_error_func_ignore12 : libjpeg_error_func12;

            if (setjmp(jerr.setjmp_buffer))
            {
                // Free memory
                delete[] jpeg_decomp;
                return img;
            }

            jpeg_create_decompress(&cinfo);

            // Parse and start decompressing
            jpeg_mem__src12(&cinfo, data, length);
            jpeg_read_header(&cinfo, FALSE);
            jpeg_start_decompress(&cinfo);

            // Init output buffer
            jpeg_decomp = new short[cinfo.image_width * cinfo.image_height];

            // Decompress
            while (cinfo.output_scanline < cinfo.output_height)
            {
                short *buffer_array[1];
                buffer_array[0] = jpeg_decomp + (cinfo.output_scanline) * cinfo.image_width;
                jpeg_read_scanlines(&cinfo, buffer_array, 1);
            }

            // Cleanup
            // jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);

            // Init CImg image
            img = Image(16, cinfo.image_width, cinfo.image_height, 1);

            // Copy over
            for (int i = 0; i < (int)cinfo.image_width * (int)cinfo.image_height; i++)
                img.set(i, jpeg_decomp[i] << 4);

            // Free memory
            delete[] jpeg_decomp;

            return img;
        }
    } // namespace image
} // namespace satdump