#include "../io.h"
#include "logger.h"
#include <cstring>
#include <csetjmp>
#include <filesystem>
extern "C"
{
#include "libs/jpeg/jpeglib.h"
}

#define INVERT_ENDIAN_16(x) ((x >> 8) | (x << 8))

namespace image
{
    struct jpeg_error_struct_l
    {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    };

    static void libjpeg_error_func_l(j_common_ptr cinfo)
    {
        longjmp(((jpeg_error_struct_l *)cinfo->err)->setjmp_buffer, 1);
    }

    void load_jpeg(Image &img, std::string file)
    {
        if (!std::filesystem::exists(file))
            return;

        FILE *fp = fopen(file.c_str(), "rb");
        if (!fp)
            abort();

        // Huge thanks to https://gist.github.com/PhirePhly/3080633
        unsigned char *jpeg_decomp[1] = { nullptr };
        jpeg_error_struct_l jerr;
        jpeg_decompress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = libjpeg_error_func_l;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp[0];
            fclose(fp);
            return;
        }

        jpeg_create_decompress(&cinfo);

        // Parse and start decompressing
        jpeg_stdio_src(&cinfo, fp);
        jpeg_read_header(&cinfo, FALSE);
        jpeg_start_decompress(&cinfo);

        // Init output buffer
        jpeg_decomp[0] = new unsigned char[cinfo.image_width * cinfo.num_components];

        // Init SatDump image
        img.init(8, cinfo.image_width, cinfo.image_height, cinfo.num_components);

        // Decompress
        while (cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, jpeg_decomp, 1);
            for (int i = 0; i < (int)cinfo.image_width; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    img.set(c, ((cinfo.output_scanline - 1) * cinfo.image_width) + i, jpeg_decomp[0][i * cinfo.num_components + c]);
        }

        // Cleanup
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        // Free memory
        delete[] jpeg_decomp[0];
        fclose(fp);
    }

    void load_jpeg(Image &img, uint8_t *buffer, int size)
    {
        // Huge thanks to https://gist.github.com/PhirePhly/3080633
        unsigned char *jpeg_decomp[1] = { nullptr };
        jpeg_error_struct_l jerr;
        jpeg_decompress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = libjpeg_error_func_l;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp[0];
            return;
        }

        jpeg_create_decompress(&cinfo);

        // Parse and start decompressing
        jpeg_mem__src(&cinfo, buffer, size);
        jpeg_read_header(&cinfo, FALSE);
        jpeg_start_decompress(&cinfo);

        // Init output buffer
        jpeg_decomp[0] = new unsigned char[cinfo.image_width * cinfo.num_components];

        // Init SatDump image
        img.init(8, cinfo.image_width, cinfo.image_height, cinfo.num_components);

        // Decompress
        while (cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, jpeg_decomp, 1);
            for (int i = 0; i < (int)cinfo.image_width; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    img.set(c, ((cinfo.output_scanline - 1) * cinfo.image_width) + i, jpeg_decomp[0][i * cinfo.num_components + c]);
        }

        // Cleanup
        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        // Free memory
        delete[] jpeg_decomp[0];
    }

    void save_jpeg(Image &img, std::string file)
    {
        auto d_depth = img.depth();
        auto d_channels = img.channels();
        auto d_height = img.height();
        auto d_width = img.width();

        if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
        {
            logger->trace("Tried to save empty JPEG!");
            return;
        }

        FILE *fp = fopen(file.c_str(), "wb");
        if (!fp)
            abort();

        unsigned char *jpeg_decomp = NULL;
        jpeg_error_struct_l jerr;
        jpeg_compress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = libjpeg_error_func_l;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp;
            fclose(fp);
            return;
        }

        jpeg_create_compress(&cinfo);

        // Parse and start decompressing
        jpeg_stdio_dest(&cinfo, fp);
        cinfo.image_width = d_width;
        cinfo.image_height = d_height;
        int jpeg_channels = d_channels == 4 ? 3 : d_channels;
        cinfo.input_components = jpeg_channels;
        cinfo.in_color_space = jpeg_channels == 3 ? JCS_RGB : JCS_GRAYSCALE;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 90, true);
        jpeg_start_compress(&cinfo, true);

        // Init output buffer
        jpeg_decomp = new unsigned char[cinfo.image_width * cinfo.image_height * cinfo.num_components];

        // Copy over
        if (d_depth == 8)
        {
            for (int i = 0; i < (int)d_width * (int)d_height; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    jpeg_decomp[i * cinfo.num_components + c] = img.get(c, i);
        }
        else if (d_depth == 16)
        {
            for (int i = 0; i < (int)d_width * (int)d_height; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    jpeg_decomp[i * cinfo.num_components + c] = img.get(c, i) >> 8; // Scale down to 8 if required
        }

        // Apply transparency, if applicable
        if (d_channels == 4)
            for (int i = 0; i < (int)d_width * (int)d_height; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    jpeg_decomp[i * cinfo.num_components + c] *= (float)img.get(3, i) / img.maxval();

        // Compress
        while (cinfo.next_scanline < cinfo.image_height)
        {
            unsigned char *buffer_array[1];
            buffer_array[0] = jpeg_decomp + (cinfo.next_scanline) * cinfo.image_width * cinfo.num_components;
            jpeg_write_scanlines(&cinfo, buffer_array, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);

        // Free memory
        fclose(fp);
        delete[] jpeg_decomp;
    }

    // Declaration is in jpeg_utils.h
    namespace
    {
        std::vector<uint8_t> j_buffer;
#define BLOCK_SIZE 16384

        void jpeg_init_destination(j_compress_ptr cinfo)
        {
            j_buffer.resize(BLOCK_SIZE);
            cinfo->dest->next_output_byte = &j_buffer[0];
            cinfo->dest->free_in_buffer = j_buffer.size();
        }

        jboolean jpeg_empty_output_buffer(j_compress_ptr cinfo)
        {
            size_t oldsize = j_buffer.size();
            j_buffer.resize(oldsize + BLOCK_SIZE);
            cinfo->dest->next_output_byte = &j_buffer[oldsize];
            cinfo->dest->free_in_buffer = j_buffer.size() - oldsize;
            return true;
        }

        void jpeg_term_destination(j_compress_ptr cinfo)
        {
            j_buffer.resize(j_buffer.size() - cinfo->dest->free_in_buffer);
        }

        std::mutex jpeg_mem_mtex;
    }

    std::vector<uint8_t> save_jpeg_mem(Image &img)
    {
        auto d_depth = img.depth();
        auto d_channels = img.channels();
        auto d_height = img.height();
        auto d_width = img.width();

        jpeg_mem_mtex.lock();
        if (img.size() == 0 || d_height == 0) // Make sure we aren't just gonna crash
        {
            logger->trace("Tried to save empty JPEG!");
            jpeg_mem_mtex.unlock();
            return std::vector<uint8_t>();
        }

        unsigned char *jpeg_decomp = NULL;
        jpeg_error_struct_l jerr;
        jpeg_compress_struct cinfo;

        // Init
        cinfo.err = jpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = libjpeg_error_func_l;

        if (setjmp(jerr.setjmp_buffer))
        {
            // Free memory
            delete[] jpeg_decomp;
            jpeg_mem_mtex.unlock();
            return std::vector<uint8_t>();
        }

        jpeg_create_compress(&cinfo);

        // Parse and start decompressing
        cinfo.dest = (struct jpeg_destination_mgr *)malloc(sizeof(struct jpeg_destination_mgr));
        cinfo.dest->init_destination = &jpeg_init_destination;
        cinfo.dest->empty_output_buffer = &jpeg_empty_output_buffer;
        cinfo.dest->term_destination = &jpeg_term_destination;

        cinfo.image_width = d_width;
        cinfo.image_height = d_height;
        int jpeg_channels = d_channels == 4 ? 3 : d_channels;
        cinfo.input_components = jpeg_channels;
        cinfo.in_color_space = jpeg_channels == 3 ? JCS_RGB : JCS_GRAYSCALE;
        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 90, true);
        jpeg_start_compress(&cinfo, true);

        // Init output buffer
        jpeg_decomp = new unsigned char[cinfo.image_width * cinfo.image_height * cinfo.num_components];

        // Copy over
        if (d_depth == 8)
        {
            for (int i = 0; i < (int)d_width * (int)d_height; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    jpeg_decomp[i * cinfo.num_components + c] = img.get(c, i);
        }
        else if (d_depth == 16)
        {
            for (int i = 0; i < (int)d_width * (int)d_height; i++)
                for (int c = 0; c < cinfo.num_components; c++)
                    jpeg_decomp[i * cinfo.num_components + c] = img.get(c, i) >> 8; // Scale down to 8 if required
        }

        // Compress
        while (cinfo.next_scanline < cinfo.image_height)
        {
            unsigned char *buffer_array[1];
            buffer_array[0] = jpeg_decomp + (cinfo.next_scanline) * cinfo.image_width * cinfo.num_components;
            jpeg_write_scanlines(&cinfo, buffer_array, 1);
        }

        jpeg_finish_compress(&cinfo);
        free(cinfo.dest);
        jpeg_destroy_compress(&cinfo);

        // Free memory
        delete[] jpeg_decomp;
        jpeg_mem_mtex.unlock();
        return j_buffer;
    }
}