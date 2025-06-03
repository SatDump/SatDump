#include "module_cryosat_siral.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
// #include <iostream>
#include <fftw3.h>

#include "image/image.h"
#include "image/io.h"

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace cryosat
{
    namespace siral
    {
        CryoSatSIRALDecoderModule::CryoSatSIRALDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
        {
            fsfsm_enable_output = false;
        }

        void CryoSatSIRALDecoderModule::process()
        {

            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/SIRAL";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directory(directory);

            uint8_t buffer[1279];

            // CCSDS Demuxer
            ccsds::ccsds_tm::Demuxer ccsdsDemuxer(1101, false, 2, 4);

            logger->info("Demultiplexing and deframing...");

            // PoseidonReader readerC, readerKU;
            std::ofstream data_out(directory + "/siral.i8");

            complex_t fft_input[243];
            complex_t fft_output[243];
            float fft_final[243];

            fftwf_plan p = fftwf_plan_dft_1d(243, (fftwf_complex *)fft_input, (fftwf_complex *)fft_output, FFTW_FORWARD, FFTW_MEASURE);

            // cimg_library::CImg<unsigned char> outputImage(243, 100000, 1, 1, 0);
            std::vector<unsigned char> fftImage(10 * 243);
            int lines = 0;

            while (should_run())
            {
                // Read buffer
                read_data((uint8_t *)buffer, 1279);

                int vcid = ccsds::ccsds_tm::parseVCDU(buffer).vcid;

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

                                // data_out.write((char *)&pkt.header.raw, 6);
                                // data_out.write((char *)pkt.payload.data(), 257);
                                //  std::cout << "SEQ " << (int)pkt.header.sequence_flag << std::endl;
                            }
                        }
                    }
                }
            }

            fftwf_free(p);

            cleanup();

            logger->info("Writing images.... (Can take a while)");

            {
                image::Image outputImage(fftImage.data(), 8, 243, lines, 1);
                image::save_img(outputImage, directory + "/SIRAL");
            }

            fftImage.clear();
        }

        void CryoSatSIRALDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("CryoSat SIRAL Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            drawProgressBar();

            ImGui::End();
        }

        std::string CryoSatSIRALDecoderModule::getID() { return "cryosat_siral"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> CryoSatSIRALDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<CryoSatSIRALDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace siral
} // namespace cryosat