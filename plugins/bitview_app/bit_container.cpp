#include "bit_container.h"
#include "common/utils.h"
#include "core/exception.h"
#include "imgui/imgui_image.h"
#include "logger.h"

#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <random>

#ifdef _WIN32
#define __USE_FILE_OFFSET64
#include "libs/mmap_windows.h"
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace satdump
{
    BitContainer::BitContainer(std::string name, std::string file_path, std::vector<FrameDef> frms) : d_name(name), d_filepath(file_path), frames(frms)
    {
        // Generate unique ID
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> check(65, 90);
        for (size_t i = 0; i < 15; i++)
            unique_id[i] = check(rng);

        // Buffer for creating textures
        wip_texture_buffer = new uint32_t[d_chunk_size * d_chunk_size];

        // Init mmap pointers
        d_file_memory_size = getFilesize(file_path);
        if (d_file_memory_size == 0)
            throw satdump_exception("Empty File!");
        fd = open(file_path.c_str(), O_RDONLY);
        d_file_memory_ptr = (uint8_t *)mmap(0, d_file_memory_size, PROT_READ, MAP_SHARED, fd, 0);
        if (d_file_memory_ptr == MAP_FAILED)
        {
            close(fd);
            throw satdump_exception("mmap failed!");
        }

        d_frame_mode = frames.size();
        logger->critical("Frame mode? %d", d_frame_mode);

        // Default period
        init_bitperiod();

        // Initial start
        update = true;
    }

    BitContainer::~BitContainer()
    {
        delete[] wip_texture_buffer;
        munmap(d_file_memory_ptr, d_file_memory_size);
        close(fd);

        if (d_is_temporary && std::filesystem::exists(d_filepath))
        {
            try
            {
                std::filesystem::remove(d_filepath);
            }
            catch (std::exception &e)
            {
                logger->warn("Failed to delete temporary file: %s", e.what());
            }
        }
    }

    void BitContainer::init_bitperiod()
    {
        if (d_frame_mode)
        {
            img_parts_y = (frames.size() / d_chunk_size) + 1;

            size_t max_size = 0;
            for (auto &f : frames)
                if (f.size > max_size)
                    max_size = f.size;

            img_parts_x = (max_size / d_chunk_size) + 1;
        }
        else
        {
            size_t final_size = 0;
            img_parts_y = 0;
            while (final_size < d_file_memory_size * 8)
            {
                final_size += d_bitperiod * d_chunk_size;
                img_parts_y++;
            }

            final_size = 0;
            img_parts_x = 0;
            while (final_size < d_bitperiod)
            {
                final_size += d_chunk_size;
                img_parts_x++;
            }
        }

        image_parts.resize(img_parts_y * img_parts_x);

        printf("%d %d\n", int(img_parts_x * d_chunk_size), int(img_parts_y * d_chunk_size));
    }

    void BitContainer::doUpdateTextures()
    {
        if (force_update_all)
        {
            for (size_t ii = 0; ii < img_parts_y; ii++)
            {
                for (size_t iii = 0; iii < img_parts_x; iii++)
                {
                    auto &part = image_parts[ii * img_parts_x + iii];
                    if (part.image_id != 0)
                        deleteImageTexture(part.image_id);
                }
            }
            force_update_all = false;
            update = true;
        }

        int max_update_num = 2;
        int updated_num = 0;

        if (update)
        {
            size_t offset = 0;

            for (size_t ii = 0; ii < img_parts_y; ii++)
            {
                for (size_t iii = 0; iii < img_parts_x; iii++)
                {
                    auto &part = image_parts[ii * img_parts_x + iii];

                    if (!part.need_update)
                        continue;
                    part.need_update = false;

                    size_t xoffset = iii * d_chunk_size;

                    if (part.visible)
                    {
                        // Only generation is slow, deleting a texture is fast
                        updated_num++;

                        if (part.image_id == 0)
                            part.image_id = makeImageTexture();

                        if (d_display_mode == 0) // Bit display
                        {
                            if (d_frame_mode)
                            {
#pragma omp parallel for
                                for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
                                {
                                    for (size_t i = 0; i < d_chunk_size; i++)
                                    {
                                        size_t frame_i = ii * d_chunk_size + line;
                                        auto &frm = frames[frame_i];

                                        size_t bitstream_pos = frm.ptr + xoffset + i;
                                        size_t raster_pos = line * d_chunk_size + i;

                                        if (bitstream_pos < d_file_memory_size * 8)
                                        {
                                            if (xoffset + i < frm.size)
                                            {
                                                uint8_t v = ((d_file_memory_ptr[bitstream_pos / 8] >> (7 - (bitstream_pos % 8))) & 1) ? 0 : 255;

                                                /*for (auto &h : highlights)
                                                {
                                                    if (h.ptr <= bitstream_pos && bitstream_pos < (h.ptr + h.size))
                                                    {
                                                        wip_texture_buffer[raster_pos] =
                                                            255 << 24 | uint8_t(v * 0.75 + h.b * 0.25) << 16 | uint8_t(v * 0.75 + h.g * 0.25) << 8 | uint8_t(v * 0.75 + h.r * 0.25);
                                                        break;
                                                    }*/
                                                wip_texture_buffer[raster_pos] = 255 << 24 | v << 16 | v << 8 | v;
                                                //}
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
                            }
                            else
                            {
#pragma omp parallel for
                                for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
                                {
                                    for (size_t i = 0; i < d_chunk_size; i++)
                                    {
                                        size_t bitstream_pos = offset + line * d_bitperiod + xoffset + i;
                                        size_t raster_pos = line * d_chunk_size + i;

                                        if (bitstream_pos < d_file_memory_size * 8)
                                        {
                                            if (xoffset + i < d_bitperiod)
                                            {
                                                uint8_t v = ((d_file_memory_ptr[bitstream_pos / 8] >> (7 - (bitstream_pos % 8))) & 1) ? 0 : 255;

                                                /*for (auto &h : highlights)
                                                {
                                                    if (h.ptr <= bitstream_pos && bitstream_pos < (h.ptr + h.size))
                                                    {
                                                        wip_texture_buffer[raster_pos] =
                                                            255 << 24 | uint8_t(v * 0.75 + h.b * 0.25) << 16 | uint8_t(v * 0.75 + h.g * 0.25) << 8 | uint8_t(v * 0.75 + h.r * 0.25);
                                                        break;
                                                    }*/
                                                wip_texture_buffer[raster_pos] = 255 << 24 | v << 16 | v << 8 | v;
                                                //}
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
                            }

                            printf("CREATE TEXT %d\n", int(ii * img_parts_x + iii));

                            updateImageTexture(part.image_id, wip_texture_buffer, d_chunk_size, d_chunk_size);
                        }
                        else if (d_display_mode == 1) // Byte display
                        {
                            if (d_frame_mode)
                            {
#pragma omp parallel for
                                for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
                                {
                                    for (size_t i = 0; i < d_chunk_size / 8; i++)
                                    {
                                        size_t frame_i = ii * d_chunk_size + line;
                                        auto &frm = frames[frame_i];

                                        size_t bitstream_pos = frm.ptr + xoffset + i * 8;
                                        size_t raster_pos = line * (d_chunk_size / 8) + i;

                                        if (bitstream_pos < d_file_memory_size * 8)
                                        {
                                            if (xoffset + i * 8 < frm.size)
                                            {
                                                uint8_t v = d_file_memory_ptr[bitstream_pos / 8];
                                                wip_texture_buffer[raster_pos] = 255 << 24 | 0 << 16 | v << 8 | 0;
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
                            }
                            else
                            {
#pragma omp parallel for
                                for (int64_t line = 0; (size_t)line < d_chunk_size; line++)
                                {
                                    for (size_t i = 0; i < d_chunk_size / 8; i++)
                                    {
                                        size_t bitstream_pos = offset + line * d_bitperiod + xoffset + i * 8;
                                        size_t raster_pos = line * (d_chunk_size / 8) + i;

                                        if (bitstream_pos < d_file_memory_size * 8)
                                        {
                                            if (xoffset + i * 8 < d_bitperiod)
                                            {
                                                uint8_t v = d_file_memory_ptr[bitstream_pos / 8];
                                                wip_texture_buffer[raster_pos] = 255 << 24 | 0 << 16 | v << 8 | 0;
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
                            }

                            // printf("CREATE TEXT %d\n", int(ii * img_parts_x + iii));

                            updateImageTexture(part.image_id, wip_texture_buffer, d_chunk_size / 8, d_chunk_size);
                        }
                    }
                    else
                    {
                        if (part.image_id != 0)
                            deleteImageTexture(part.image_id);
                        part.image_id = 0;
                    }

                    part.pos1_x = d_chunk_size * (iii + 0);
                    part.pos1_y = d_chunk_size * (ii + 1);
                    part.pos2_x = d_chunk_size * (iii + 1);
                    part.pos2_y = d_chunk_size * (ii + 0);
                    part.i = ii * img_parts_x + iii;

                    if (updated_num >= max_update_num)
                        return; // TODOREWORK?
                }

                offset += d_bitperiod * d_chunk_size;
            }

            update = false;
        }
    }

    void BitContainer::doDrawPlotTextures(ImPlotRect c)
    {
        for (auto &part : image_parts)
        {
            // if (c.Min().x > part.pos1_x || c.Max().y < part.pos1_y)
            //     continue;
            // if (c.Max().x < part.pos2_x || c.Min().y > part.pos2_y)
            //     continue;

            if (part.i == -1)
            {
                part.need_update = true;
                continue;
            }

            bool status_before = part.visible;
            part.visible = false;
            if (c.Min().x > part.pos2_x || c.Max().y < part.pos2_y) {}
            else if (c.Max().x < part.pos1_x || c.Min().y > part.pos1_y) {}
            else if (!part.need_update)
            {
                // printf("%f %f - %f %f  ----  %f %f - %f %f ---- %d\n",
                //        c.Min().x, c.Min().y, c.Max().x, c.Max().y,
                //        part.pos1_x, part.pos1_y, part.pos2_x, part.pos2_y,
                //        part.i);
                ImPlot::PlotImage("Test", (void *)(intptr_t)part.image_id, {part.pos1_x, part.pos1_y}, {part.pos2_x, part.pos2_y});

                part.visible = true;
            }

            if (part.visible != status_before)
            {
                part.need_update = true;
                update = true;
            }
        }
    }
} // namespace satdump
