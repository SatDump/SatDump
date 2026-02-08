#include "bit_container.h"
#include "core/resources.h"
#include "image/image.h"
#include "image/text.h"
#include "imgui/imgui_image.h"
#include <cstdint>

namespace satdump
{
    void BitContainer::renderSegmentText(PartImage &part, size_t &ii, size_t &iii, size_t &xoffset, size_t &offset)
    {
        int bits = 8;

        image::Image img(8, d_chunk_size, d_chunk_size * 14, 4);
        image::TextDrawer text_draw;
        text_draw.init_font(resources::getResourcePath("fonts/monospace.medium.ttf"));

        for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
        {
            for (size_t i = 0; i < d_chunk_size / bits; i++)
            {
                size_t bitstream_pos;
                size_t raster_pos;
                size_t max_size;

                if (d_frame_mode)
                {
                    size_t frame_i = ii * d_chunk_size + line;
                    auto &frm = frames[frame_i];

                    bitstream_pos = frm.ptr + xoffset + i * bits;
                    raster_pos = line * (d_chunk_size / bits) + i;
                    max_size = frm.size;
                }
                else
                {
                    bitstream_pos = offset + line * d_bitperiod + xoffset + i * bits;
                    raster_pos = line * (d_chunk_size / bits) + i;
                    max_size = d_bitperiod;
                }

                if (bitstream_pos < d_file_memory_size * 8)
                {
                    if (xoffset + i * bits < max_size)
                    {
                        uint32_t vv = 0;
                        for (int b = 0; b < bits; b++)
                            vv = vv << 1 | ((d_file_memory_ptr[(bitstream_pos + b) / 8] >> (7 - ((bitstream_pos + b) % 8))) & 1);

                        uint8_t v = bits > 8 ? (vv >> (bits - 8)) : (vv << (8 - bits));

                        // wip_texture_buffer[raster_pos] = 255 << 24 | 0 << 16 | v << 8 | 0;
                        text_draw.draw_text(img, i * 8, line * 14 - 6, {1, 1, 1, 1}, 14, std::string(((char *)&v), 1));

#if 0
                        for (auto &h : highlights)
                        {
                            if (h.ptr <= bitstream_pos && bitstream_pos < (h.ptr + h.size))
                            {
                                wip_texture_buffer[raster_pos] = 255 << 24 | uint8_t(v * 0.75 + h.b * 0.25) << 16 | uint8_t(v * 0.75 + h.g * 0.25) << 8 | uint8_t(v * 0.75 + h.r * 0.25);
                                break;
                            }
                        }
#endif
                    }
                }
            }
        }

        uint32_t *wip_texture_buffer2 = new uint32_t[d_chunk_size * d_chunk_size * 14 * 4];
        image::image_to_rgba(img, wip_texture_buffer2);
        updateImageTexture(part.image_id, wip_texture_buffer2, img.width(), img.height());
        delete[] wip_texture_buffer2;
    }

    void BitContainer::renderSegment(PartImage &part, size_t &ii, size_t &iii, size_t &xoffset, size_t &offset)
    {
        // ASCII Mode
        if (d_display_mode == 1)
        {
            renderSegmentText(part, ii, iii, xoffset, offset);
            return;
        }

        int &bits = d_display_bits;

        if (bits == 1)
        {
            for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
            {
                for (size_t i = 0; i < d_chunk_size; i++)
                {
                    size_t bitstream_pos;
                    size_t raster_pos;
                    size_t max_size;

                    if (d_frame_mode)
                    {
                        size_t frame_i = ii * d_chunk_size + line;
                        auto &frm = frames[frame_i];

                        bitstream_pos = frm.ptr + xoffset + i;
                        raster_pos = line * d_chunk_size + i;
                        max_size = frm.size;
                    }
                    else
                    {
                        bitstream_pos = offset + line * d_bitperiod + xoffset + i;
                        raster_pos = line * d_chunk_size + i;
                        max_size = d_bitperiod;
                    }

                    if (bitstream_pos < d_file_memory_size * 8)
                    {
                        if (xoffset + i < max_size)
                        {
                            uint8_t v = ((d_file_memory_ptr[bitstream_pos / 8] >> (7 - (bitstream_pos % 8))) & 1) ? 0 : 255;

                            wip_texture_buffer[raster_pos] = 255 << 24 | v << 16 | v << 8 | v;

#if 0
                        for (auto &h : highlights)
                        {
                            if (h.ptr <= bitstream_pos && bitstream_pos < (h.ptr + h.size))
                            {
                                wip_texture_buffer[raster_pos] = 255 << 24 | uint8_t(v * 0.75 + h.b * 0.25) << 16 | uint8_t(v * 0.75 + h.g * 0.25) << 8 | uint8_t(v * 0.75 + h.r * 0.25);
                                break;
                            }
                        }
#endif
                        }
                        else
                        {
                            wip_texture_buffer[raster_pos] = 0 << 24;
                        }
                    }
                    else
                    {
                        wip_texture_buffer[raster_pos] = 0 << 24;
                    }
                }
            }

            updateImageTexture(part.image_id, wip_texture_buffer, d_chunk_size, d_chunk_size);
        }
        else
        {
            for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
            {
                for (size_t i = 0; i < d_chunk_size / bits; i++)
                {
                    size_t bitstream_pos;
                    size_t raster_pos;
                    size_t max_size;

                    if (d_frame_mode)
                    {
                        size_t frame_i = ii * d_chunk_size + line;
                        auto &frm = frames[frame_i];

                        bitstream_pos = frm.ptr + xoffset + i * bits;
                        raster_pos = line * (d_chunk_size / bits) + i;
                        max_size = frm.size;
                    }
                    else
                    {
                        bitstream_pos = offset + line * d_bitperiod + xoffset + i * bits;
                        raster_pos = line * (d_chunk_size / bits) + i;
                        max_size = d_bitperiod;
                    }

                    if (bitstream_pos < d_file_memory_size * 8)
                    {
                        if (xoffset + i * bits < max_size)
                        {
                            uint32_t vv = 0;
                            for (int b = 0; b < bits; b++)
                                vv = vv << 1 | ((d_file_memory_ptr[(bitstream_pos + b) / 8] >> (7 - ((bitstream_pos + b) % 8))) & 1);

                            uint8_t v = bits > 8 ? (vv >> (bits - 8)) : (vv << (8 - bits));

                            wip_texture_buffer[raster_pos] = 255 << 24 | 0 << 16 | v << 8 | 0;

#if 0
                        for (auto &h : highlights)
                        {
                            if (h.ptr <= bitstream_pos && bitstream_pos < (h.ptr + h.size))
                            {
                                wip_texture_buffer[raster_pos] = 255 << 24 | uint8_t(v * 0.75 + h.b * 0.25) << 16 | uint8_t(v * 0.75 + h.g * 0.25) << 8 | uint8_t(v * 0.75 + h.r * 0.25);
                                break;
                            }
                        }
#endif
                        }
                        else
                        {
                            wip_texture_buffer[raster_pos] = 0 << 24;
                        }
                    }
                    else
                    {
                        wip_texture_buffer[raster_pos] = 0 << 24;
                    }
                }
            }

            updateImageTexture(part.image_id, wip_texture_buffer, d_chunk_size / bits, d_chunk_size);
        }
    }
} // namespace satdump