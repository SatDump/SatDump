#include "uhrit_demux.h"
#include "common/ccsds/ccsds.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "logger.h"
#include "utils/time.h"
#include "xrit/xrit_file.h"
#include <cstdint>
#include <exception>
#include <fstream>
#include <string>
#include <vector>

namespace satdump
{
    namespace xrit
    {
        UHRITDemux::UHRITDemux(int mpdu_size) : d_mpdu_size(mpdu_size) {}

        UHRITDemux::~UHRITDemux() {}

        std::vector<XRITFile> UHRITDemux::work(uint8_t *cadu)
        {
            files.clear();

            ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

            if (vcdu.vcid == 63) // Skip filler
                return files;

            if (demuxers.count(vcdu.vcid) <= 0) // Add new demux if required
            {
                demuxers.emplace(std::pair<int, std::unique_ptr<ccsds::ccsds_aos::Demuxer>>(vcdu.vcid, std::make_unique<ccsds::ccsds_aos::Demuxer>(d_mpdu_size, false, 0, true)));
                wip_files.insert({vcdu.vcid, std::map<int, std::vector<uint8_t>>()});
            }

            // Demux
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxers[vcdu.vcid]->work(cadu);

            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 2047 || pkt.payload.size() < 4) // Skip filler
                    continue;

                if (wip_files[vcdu.vcid].count(pkt.header.apid) == 0) // One file per APID
                    wip_files[vcdu.vcid].insert({pkt.header.apid, std::vector<uint8_t>()});

                auto &current_file = wip_files[vcdu.vcid][pkt.header.apid];

                // Check CRC
                if (!ccsds::crcCheckHLDC32(pkt))
                {
                    logger->error("LRIT CRC is invalid... Skipping. %d - %d %d - %d", pkt.header.apid, pkt.header.packet_length, pkt.payload.size(), pkt.header.packet_sequence_count);
                    current_file.clear();
                    continue;
                }

                if (pkt.header.sequence_flag == 1 || pkt.header.sequence_flag == 3)
                {
                    if (current_file.size())
                        finalizeLRITData(current_file);

                    if (pkt.payload.size() >= 12)
                        current_file.insert(current_file.end(), pkt.payload.begin() + 8, pkt.payload.end() - 4);
                }
                else if (pkt.header.sequence_flag == 0)
                {
                    if (current_file.size() && pkt.payload.size() >= 12)
                        current_file.insert(current_file.end(), pkt.payload.begin() + 8, pkt.payload.end() - 4);
                }
                else if (pkt.header.sequence_flag == 2)
                {
                    if (current_file.size() && pkt.payload.size() >= 12)
                        current_file.insert(current_file.end(), pkt.payload.begin() + 8, pkt.payload.end() - 4);
                    if (current_file.size())
                        finalizeLRITData(current_file);
                }
            }

            return files;
        }

        void UHRITDemux::finalizeLRITData(std::vector<uint8_t> &file)
        {
            if (file.size() < 10)
                return;

            try
            {
                uint16_t file_counter = file[0] << 8 | file[1];
                uint64_t file_size = (uint64_t)file[2] << 56 | //
                                     (uint64_t)file[3] << 48 | //
                                     (uint64_t)file[4] << 40 | //
                                     (uint64_t)file[5] << 32 | //
                                     (uint64_t)file[6] << 24 | //
                                     (uint64_t)file[7] << 16 | //
                                     (uint64_t)file[8] << 8 |  //
                                     (uint64_t)file[9];

                // file.filename = std::to_string(file_counter);
                // file.lrit_data.erase(file.lrit_data.begin(), file.lrit_data.begin() + 10);

                file.erase(file.begin(), file.begin() + 10);

                // logger->critical("FINAL FILE %d %llu %llu %d MISSING %d", file_counter, file_size / 8, file.size(), (file_size / 8) - file.size(), file.size());

                //  file.resize(81920);
                // dataout.write((char *)file.data() + file.size() - 100, 100); // file.size());

                XRITFile xfile;
                xfile.lrit_data.swap(file);
                xfile.parseHeaders(true);

                if (xfile.hasHeader<AnnotationRecord>())
                {
                    //   logger->critical("---------------FINAL FILE %d %llu %llu %d (%d)", file_counter, file_size / 8, xfile.lrit_data.size(), (file_size / 8) - xfile.lrit_data.size(),
                    //                    (xfile.getHeader<PrimaryHeader>().data_field_length / 8) - xfile.lrit_data.size());
                    logger->info("New LRIT file : " + xfile.filename);

                    onParseHeader(xfile);

                    if (xfile.filename.size())
                        files.push_back(xfile);
                }
            }
            catch (std::exception &e)
            {
                logger->error("Error parsing UHRIT file : %s", e.what());
            }

            file.clear();
        }
    } // namespace xrit
} // namespace satdump