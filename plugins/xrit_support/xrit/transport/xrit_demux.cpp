#include "xrit_demux.h"
#include "common/ccsds/ccsds_aos/vcdu.h"
#include "crc_table.h"
#include "logger.h"

namespace satdump
{
    namespace xrit
    {
        XRITDemux::XRITDemux(int mpdu_size, bool check_crc) : d_mpdu_size(mpdu_size), d_check_crc(check_crc) {}

        XRITDemux::~XRITDemux() {}

        // CRC Implementation from LRIT-Mission-Specific-Document.pdf
        uint16_t computeCRC(const uint8_t *data, int size)
        {
            uint16_t crc = 0xffff;
            for (int i = 0; i < size; i++)
                crc = (crc << 8) ^ crc_table[(crc >> 8) ^ (uint16_t)data[i]];
            return crc;
        }

        std::vector<XRITFile> XRITDemux::work(uint8_t *cadu)
        {
            files.clear();

            ccsds::ccsds_aos::VCDU vcdu = ccsds::ccsds_aos::parseVCDU(cadu);

            if (vcdu.vcid == 63) // Skip filler
                return files;

            if (demuxers.count(vcdu.vcid) <= 0) // Add new demux if required
            {
                demuxers.emplace(std::pair<int, std::unique_ptr<ccsds::ccsds_aos::Demuxer>>(vcdu.vcid, std::make_unique<ccsds::ccsds_aos::Demuxer>(d_mpdu_size, false)));
                wip_files.insert({vcdu.vcid, std::map<int, XRITFile>()});
            }

            // Demux
            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxers[vcdu.vcid]->work(cadu);

            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 2047 || pkt.payload.size() < 2) // Skip filler
                    continue;

                if (wip_files[vcdu.vcid].count(pkt.header.apid) == 0) // One file per APID
                    wip_files[vcdu.vcid].insert({pkt.header.apid, XRITFile()});

                XRITFile &current_file = wip_files[vcdu.vcid][pkt.header.apid];

                // Check CRC
                uint16_t crc = pkt.payload.data()[pkt.payload.size() - 2] << 8 | pkt.payload.data()[pkt.payload.size() - 1];
                if (d_check_crc ? !(crc == computeCRC(pkt.payload.data(), pkt.payload.size() - 2)) : false)
                {
                    // If this is the middle of a file, we have the header, and it's an image, continue on failure
                    bool can_continue = false;
                    if (current_file.file_in_progress && pkt.header.sequence_flag == 0 && current_file.header_parsed)
                    {
                        PrimaryHeader primary_header = current_file.getHeader<PrimaryHeader>();
                        can_continue = primary_header.file_type_code == 0 && current_file.hasHeader<ImageStructureRecord>();
                    }

                    if (can_continue)
                    {
                        logger->warn("LRIT CRC is invalid, but file can be recovered");
                        processLRITData(current_file, pkt, true);
                    }
                    else
                    {
                        logger->error("LRIT CRC is invalid... Skipping.");
                        current_file.file_in_progress = false;
                        std::vector<uint8_t>().swap(current_file.lrit_data);
                    }
                    continue;
                }

                if (pkt.header.sequence_flag == 1 || pkt.header.sequence_flag == 3)
                {
                    if (current_file.file_in_progress)
                        finalizeLRITData(current_file);

                    current_file.lrit_data.clear();

                    processLRITHeader(current_file, pkt);
                    current_file.vcid = vcdu.vcid;
                    current_file.header_parsed = false;
                    current_file.file_in_progress = true;
                    current_file.last_tracked_counter = pkt.header.packet_sequence_count;
                }
                else if (pkt.header.sequence_flag == 0)
                {
                    if (current_file.file_in_progress)
                        processLRITData(current_file, pkt);
                }
                else if (pkt.header.sequence_flag == 2)
                {
                    if (current_file.file_in_progress)
                    {
                        processLRITData(current_file, pkt);
                        finalizeLRITData(current_file);
                        current_file.file_in_progress = false;
                        std::vector<uint8_t>().swap(current_file.lrit_data);
                    }
                }

                if (current_file.file_in_progress && !current_file.header_parsed)
                {
                    PrimaryHeader primary_header = current_file.getHeader<PrimaryHeader>();

                    if (current_file.lrit_data.size() >= primary_header.total_header_length)
                    {
                        parseHeader(current_file);
                        current_file.header_parsed = true;
                        if (pkt.header.sequence_flag == 3)
                        {
                            finalizeLRITData(current_file);
                            current_file.file_in_progress = false;
                            std::vector<uint8_t>().swap(current_file.lrit_data);
                        }
                    }
                }
            }

            return files;
        }

        void XRITDemux::processLRITHeader(XRITFile &file, ccsds::CCSDSPacket &pkt)
        {
            file.lrit_data.insert(file.lrit_data.end(), &pkt.payload.data()[10], &pkt.payload.data()[pkt.payload.size() - 2]);
        }

        void XRITDemux::parseHeader(XRITFile &file)
        {
            file.parseHeaders();
            logger->info("New LRIT file : " + file.filename);
            onParseHeader(file);
        }

        void XRITDemux::processLRITData(XRITFile &file, ccsds::CCSDSPacket &pkt, bool bad_crc)
        {
            if (onProcessData(file, pkt, bad_crc))
                file.lrit_data.insert(file.lrit_data.end(), &pkt.payload.data()[0], &pkt.payload.data()[pkt.payload.size() - 2]);
        }

        void XRITDemux::finalizeLRITData(XRITFile &file)
        {
            onFinalizeData(file);
            files.push_back(file);
        }
    } // namespace xrit
} // namespace satdump