#include "module_himawaricast_data_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "common/utils.h"
#include "common/mpeg_ts/ts_header.h"
#include "common/mpeg_ts/ts_demux.h"
#include "common/mpeg_ts/fazzt_processor.h"
#include "lrit_header.h"
#include "common/codings/dvb-s2/bbframe_ts_parser.h"
#include "libs/bzlib/bzlib.h"

namespace himawari
{
    namespace himawaricast
    {
        HimawariCastDataDecoderModule::HimawariCastDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        std::vector<ModuleDataType> HimawariCastDataDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> HimawariCastDataDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        HimawariCastDataDecoderModule::~HimawariCastDataDecoderModule()
        {
            for (std::pair<const std::string, SegmentedLRITImageDecoder> &dec : segmented_decoders)
            {
                std::string channel_name = dec.first;
                std::string current_filename = segmented_decoders_filenames[channel_name];
                segmented_decoders[channel_name].image.normalize();
                if (!std::filesystem::exists(directory + "/" + channel_name))
                    std::filesystem::create_directory(directory + "/" + channel_name);
                segmented_decoders[channel_name].image.save_img(directory + "/" + channel_name + "/" + current_filename.substr(0, current_filename.size() - 4));
                // segmented_decoders[channel_name].image.clear();
                // segmented_decoders.erase(channel_name);
                // segmented_decoders_filenames.erase(channel_name);
            }
        }

        // Slightly modified version of the original,
        // to return how many bytes were consumed as well,
        // so we can iterate over a few concatenated files
        int BZ2_bzBuffToBuffDecompress_M(char *dest,
                                         unsigned int *destLen,
                                         char *source,
                                         unsigned int sourceLen,
                                         unsigned int *consumedSource,
                                         int small,
                                         int verbosity)
        {
            bz_stream strm;
            int ret;

            if (dest == NULL || destLen == NULL ||
                source == NULL ||
                (small != 0 && small != 1) ||
                verbosity < 0 || verbosity > 4)
                return BZ_PARAM_ERROR;

            strm.bzalloc = NULL;
            strm.bzfree = NULL;
            strm.opaque = NULL;
            ret = BZ2_bzDecompressInit(&strm, verbosity, small);
            if (ret != BZ_OK)
                return ret;

            strm.next_in = source;
            strm.next_out = dest;
            strm.avail_in = sourceLen;
            strm.avail_out = *destLen;

            ret = BZ2_bzDecompress(&strm);
            if (ret == BZ_OK)
                goto output_overflow_or_eof;
            if (ret != BZ_STREAM_END)
                goto errhandler;

            /* normal termination */
            *destLen -= strm.avail_out;
            *consumedSource = sourceLen - strm.avail_in;
            BZ2_bzDecompressEnd(&strm);
            return BZ_OK;

        output_overflow_or_eof:
            if (strm.avail_out > 0)
            {
                BZ2_bzDecompressEnd(&strm);
                return BZ_UNEXPECTED_EOF;
            }
            else
            {
                BZ2_bzDecompressEnd(&strm);
                return BZ_OUTBUFF_FULL;
            };

        errhandler:
            BZ2_bzDecompressEnd(&strm);
            return ret;
        }

        void HimawariCastDataDecoderModule::process()
        {
            std::ifstream data_in;

            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);

            directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/'));

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            bool ts_input = d_parameters["ts_input"].get<bool>();

            // BBFrame stuff
            uint8_t bb_frame[38688];
            dvbs2::BBFrameTSParser ts_extractor(38688);

            // TS Stuff
            int ts_cnt = 1;
            uint8_t mpeg_ts_all[188 * 1000];
            mpeg_ts::TSDemux ts_demux;
            fazzt::FazztProcessor fazzt_processor(1411);

            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                if (ts_input)
                {
                    // Read buffer
                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)mpeg_ts_all, 188);
                    else
                        input_fifo->read((uint8_t *)mpeg_ts_all, 188);
                    ts_cnt = 1;
                }
                else
                {
                    // Read buffer
                    if (input_data_type == DATA_FILE)
                        data_in.read((char *)bb_frame, 38688 / 8);
                    else
                        input_fifo->read((uint8_t *)bb_frame, 38688 / 8);
                    ts_cnt = ts_extractor.work(bb_frame, 1, mpeg_ts_all, 188 * 1000);
                }

                for (int tsc = 0; tsc < ts_cnt; tsc++)
                {
                    uint8_t *mpeg_ts = &mpeg_ts_all[tsc * 188];
                    std::vector<std::vector<uint8_t>> frames = ts_demux.demux(mpeg_ts, 1001);

                    for (std::vector<uint8_t> &payload : frames)
                    {
                        payload.erase(payload.begin(), payload.begin() + 40); // Extract the Fazzt frame

                        std::vector<fazzt::FazztFile> files = fazzt_processor.work(payload);

                        for (fazzt::FazztFile &file : files)
                        {
                            size_t buffer_output_size = 50 * 1e6; // 50MB. Should be enough
                            size_t total_output_size = 0;
                            std::vector<uint8_t> out_buffer(buffer_output_size);
                            bool decomp_error = false;
                            uint8_t *out_ptr = out_buffer.data();

                            // Iterate through all compressed blocks... There can be more than one (with, what the heck, each a file signature...)!
                            for (size_t i = 0; i < file.size;)
                            {
                                char *file_c = (char *)file.data.data();
                                if (file_c[i + 0] == 'B' &&
                                    file_c[i + 1] == 'Z' &&
                                    file_c[i + 2] == 'h')
                                {
                                    unsigned int local_outsize = buffer_output_size;
                                    unsigned int consumed;

                                    int ret = BZ2_bzBuffToBuffDecompress_M((char *)out_ptr, &local_outsize, (char *)&file.data[i], file.data.size() - i, &consumed, false, 1);
                                    if (ret != BZ_OK && ret != BZ_STREAM_END)
                                    {
                                        decomp_error = true;
                                        logger->error("Failed decompressing Bzip2 data! Error : " + std::to_string(ret));
                                    }

                                    buffer_output_size -= local_outsize;
                                    out_ptr += local_outsize;
                                    total_output_size += local_outsize;
                                    i += consumed;
                                }
                                else
                                    i++; // Just in case...
                            }

                            // Write it all to the file
                            out_buffer.resize(total_output_size);
                            file.data = out_buffer;
                            file.size = total_output_size;
                            out_buffer.clear();

                            // Then carry on with the processing
                            if (decomp_error)
                            {
                                logger->warn("Error decompressing, discarding file...");
                            }
                            else
                            {
                                try
                                {
                                    if (file.name.find("IMG_") != std::string::npos)
                                        file.name = (file.name.substr(0, file.name.size() - 4)) + ".hrit";
                                    else
                                        file.name = (file.name.substr(0, file.name.size() - 4));

                                    if (file.name.find("IMG_") != std::string::npos)
                                    {
                                        auto lrit_data = file.data;
                                        PrimaryHeader primary_header(&lrit_data[0]);

                                        if (lrit_data.size() != (primary_header.data_field_length / 8) + primary_header.total_header_length)
                                            continue;

                                        // Get all other headers
                                        std::map<int, int> all_headers;
                                        for (uint32_t i = 0; i < primary_header.total_header_length;)
                                        {
                                            uint8_t type = lrit_data[i];
                                            uint16_t record_length = lrit_data[i + 1] << 8 | lrit_data[i + 2];

                                            if (record_length == 0)
                                                break;

                                            all_headers.emplace(std::pair<int, int>(type, i));

                                            i += record_length;
                                        }

                                        // Check if this has a filename
                                        if (all_headers.count(AnnotationRecord::TYPE) > 0)
                                        {
                                            AnnotationRecord annotation_record(&lrit_data[all_headers[AnnotationRecord::TYPE]]);

                                            std::string current_filename = std::string(annotation_record.annotation_text.data());

                                            std::replace(current_filename.begin(), current_filename.end(), '/', '_');  // Safety
                                            std::replace(current_filename.begin(), current_filename.end(), '\\', '_'); // Safety

                                            logger->info("New xRIT file : " + current_filename);

                                            // Check if this is image data
                                            if (all_headers.count(ImageStructureRecord::TYPE) > 0)
                                            {
                                                ImageStructureRecord image_structure_record(&lrit_data[all_headers[ImageStructureRecord::TYPE]]);
                                                logger->debug("This is image data. Size " + std::to_string(image_structure_record.columns_count) + "x" + std::to_string(image_structure_record.lines_count));

                                                image::Image<uint16_t> image;

                                                if (image_structure_record.bit_per_pixel == 8)
                                                {
                                                    image = image::Image<uint8_t>(&lrit_data[primary_header.total_header_length],
                                                                                  image_structure_record.columns_count,
                                                                                  image_structure_record.lines_count, 1)
                                                                .to16bits();
                                                }
                                                else if (image_structure_record.bit_per_pixel == 16)
                                                {
                                                    image::Image<uint16_t> image2(image_structure_record.columns_count,
                                                                                  image_structure_record.lines_count, 1);

                                                    for (long long int i = 0; i < image_structure_record.columns_count * image_structure_record.lines_count; i++)
                                                        image2[i] = ((&lrit_data[primary_header.total_header_length])[i * 2 + 0] << 8 |
                                                                     (&lrit_data[primary_header.total_header_length])[i * 2 + 1]);

                                                    image = image2;
                                                }

                                                std::string channel_name = current_filename.substr(4, 7);
                                                int segment = std::stoi(current_filename.substr(current_filename.size() - 3, current_filename.size())) - 1;
                                                long id = std::stoll(current_filename.substr(12, 12));

                                                logger->debug("Channel %s segment %d id %d", channel_name.c_str(), segment, id);

                                                if (segmented_decoders[channel_name].image_id != id || segmented_decoders[channel_name].isComplete())
                                                {
                                                    segmented_decoders[channel_name].image.normalize();
                                                    if (!std::filesystem::exists(directory + "/" + channel_name))
                                                        std::filesystem::create_directory(directory + "/" + channel_name);
                                                    segmented_decoders[channel_name].image.save_img(directory + "/" + channel_name + "/" + current_filename.substr(0, current_filename.size() - 4));
                                                    segmented_decoders[channel_name].image.clear();
                                                    segmented_decoders.erase(channel_name);
                                                    segmented_decoders_filenames.erase(channel_name);
                                                }

                                                if (segmented_decoders.count(channel_name) == 0)
                                                {
                                                    segmented_decoders.insert({channel_name, SegmentedLRITImageDecoder(10, image_structure_record.columns_count, image_structure_record.lines_count, id)});
                                                    segmented_decoders_filenames.insert({channel_name, current_filename});
                                                }

                                                segmented_decoders[channel_name].pushSegment(image.data(), segment);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        logger->debug("Saving " + file.name + " size " + std::to_string(file.size));

                                        std::ofstream output_himawari_file(directory + "/" + file.name);
                                        output_himawari_file.write((char *)file.data.data(), file.data.size());
                                        output_himawari_file.close();
                                    }
                                }
                                catch (std::exception &e)
                                {
                                    logger->error("Error processing HimawariCast file %s", e.what());
                                }
                            }
                        }
                    }
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
                }
            }

            data_in.close();
        }

        void HimawariCastDataDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("HimawariCast Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string HimawariCastDataDecoderModule::getID()
        {
            return "himawaricast_data_decoder";
        }

        std::vector<std::string> HimawariCastDataDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> HimawariCastDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<HimawariCastDataDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace avhrr
} // namespace metop