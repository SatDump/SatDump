#pragma once

#include "common/ccsds/ccsds_aos/demuxer.h"
#include "xrit/xrit_file.h"
#include <functional>
#include <memory>

namespace satdump
{
    namespace xrit
    {
        class XRITDemux
        {
        private:
            const int d_mpdu_size;
            const bool d_check_crc;

        private:
            std::map<int, std::unique_ptr<ccsds::ccsds_aos::Demuxer>> demuxers;
            std::map<int, std::map<int, XRITFile>> wip_files;

            std::vector<XRITFile> files;

        private:
            void processLRITHeader(XRITFile &file, ccsds::CCSDSPacket &pkt);
            void parseHeader(XRITFile &file);
            void processLRITData(XRITFile &file, ccsds::CCSDSPacket &pkt, bool bad_crc = false);
            void finalizeLRITData(XRITFile &file);

        public:
            std::function<void(XRITFile &)> onParseHeader = [](XRITFile &) -> void {};
            std::function<bool(XRITFile &, ccsds::CCSDSPacket &, bool)> onProcessData = [](XRITFile &, ccsds::CCSDSPacket &, bool) -> bool { return true; };
            std::function<void(XRITFile &)> onFinalizeData = [](XRITFile &) -> void {};

        public:
            XRITDemux(int mpdu_size = 884, bool check_crc = true);
            ~XRITDemux();

            std::vector<XRITFile> work(uint8_t *cadu);
        };
    } // namespace xrit
} // namespace satdump