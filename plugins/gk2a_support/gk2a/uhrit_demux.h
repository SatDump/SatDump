#pragma once

#include "common/ccsds/ccsds_aos/demuxer.h"
#include "xrit/xrit_file.h"
#include <functional>
#include <memory>

namespace satdump
{
    namespace xrit
    {
        class UHRITDemux
        {
        private:
            const int d_mpdu_size;

        private:
            std::map<int, std::unique_ptr<ccsds::ccsds_aos::Demuxer>> demuxers;
            std::map<int, std::map<int, std::vector<uint8_t>>> wip_files;

            std::vector<XRITFile> files;

        private:
            void finalizeLRITData(std::vector<uint8_t> &file);

        public:
            std::function<void(XRITFile &)> onParseHeader = [](XRITFile &) -> void {};

        public:
            UHRITDemux(int mpdu_size = 2034);
            ~UHRITDemux();

            std::vector<XRITFile> work(uint8_t *cadu);
        };
    } // namespace xrit
} // namespace satdump