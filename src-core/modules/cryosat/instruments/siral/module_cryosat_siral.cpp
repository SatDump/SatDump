#include "module_cryosat_siral.h"
#include <fstream>
#include "common/ccsds/ccsds_1_0_jason/demuxer.h"
#include "common/ccsds/ccsds_1_0_jason/vcdu.h"
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
//#include <iostream>
#include <fftw3.h>

#include "common/image/image.h"
#include "common/resizeable_buffer.h"

// Return filesize
size_t getFilesize(std::string filepath);

namespace cryosat
{
    namespace siral
    {
        CryoSatSIRALDecoderModule::CryoSatSIRALDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
        }

        void CryoSatSIRALDecoderModule::process()
        {
            filesize = getFilesize(d_input_file);
            std::ifstream data_in(d_input_file, std::ios::binary);

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SIRAL";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            logger->info("Using input frames " + d_input_file);
            logger->info("Decoding to " + directory);

            time_t lastTime = 0;

            uint8_t buffer[1279];

            // CCSDS Demuxer
            ccsds::ccsds_1_0_jason::Demuxer ccsdsDemuxer(1101, false);

            logger->info("Demultiplexing and deframing...");

            //PoseidonReader readerC, readerKU;
            std::ofstream data_out(directory + "/siral.i8");

            complex_t fft_input[243];
            complex_t fft_output[243];
            float fft_final[243];

            fftwf_plan p = fftwf_plan_dft_1d(243, (fftwf_complex *)fft_input, (fftwf_complex *)fft_output, FFTW_FORWARD, FFTW_MEASURE);

            //cimg_library::CImg<unsigned char> outputImage(243, 100000, 1, 1, 0);
            ResizeableBuffer<unsigned char> fftImage;
            fftImage.create(100000 * 243);
            int lines = 0;

            while (!data_in.eof())
            {
                // Read buffer
                data_in.read((char *)buffer, 1279);

                int vcid = ccsds::ccsds_1_0_jason::parseVCDU(buffer).vcid;

                // std::cout << "VCID " << vcid << std::endl;

                if (vcid == 4)
                {
                    std::vector<ccsds::CCSDSPacket> pkts = ccsdsDemuxer.work(buffer);

                    if (pkts.size() > 0)
                    {
                        for (ccsds::CCSDSPacket pkt : pkts)
                        {
                            if (pkt.header.apid == 2047)
                                continue;

                            // std::cout << "APID " << pkt.header.apid << std::endl;

                            if (pkt.header.apid == 19)
                            {
                                // std::cout << "LEN " << pkt.payload.size() << std::endl;
                                data_out.write((char *)&pkt.payload.data()[14], 257 - 14);

                                volk_8i_s32f_convert_32f((float *)fft_input, (int8_t *)&pkt.payload.data()[14], 127, 243);

                                fftwf_execute(p);

                                volk_32fc_s32f_x2_power_spectral_density_32f(fft_final, (lv_32fc_t *)fft_output, 1, 1, 243);

                                for (int i = 0; i < 121; i++)
                                    fftImage[lines * 243 + 121 + i] = std::max<int>(0, std::min<int>(255, (fft_final[i] + 100)));
                                for (int i = 121; i < 243; i++)
                                    fftImage[lines * 243 + i - 121] = std::max<int>(0, std::min<int>(255, (fft_final[i] + 100)));
                                lines++;

                                if (lines * 243 >= (int)fftImage.size())
                                    fftImage.resize((lines + 10000) * 243);

                                //data_out.write((char *)&pkt.header.raw, 6);
                                //data_out.write((char *)pkt.payload.data(), 257);
                                // std::cout << "SEQ " << (int)pkt.header.sequence_flag << std::endl;
                            }
                        }
                    }
                }

                progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
                }
            }

            fftwf_free(p);

            data_in.close();

            logger->info("Writing images.... (Can take a while)");

            {
                image::Image<uint8_t> outputImage(fftImage.buf, 243, lines, 1);
                WRITE_IMAGE(outputImage, directory + "/SIRAL.png");
            }

            fftImage.destroy();
        }

        void CryoSatSIRALDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("CryoSat SIRAL Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string CryoSatSIRALDecoderModule::getID()
        {
            return "cryosat_siral";
        }

        std::vector<std::string> CryoSatSIRALDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> CryoSatSIRALDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CryoSatSIRALDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace swap
} // namespace proba