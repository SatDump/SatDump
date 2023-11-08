#pragma once

#include "lrit_file.h"
#include "common/ccsds/ccsds_weather/demuxer.h"
#include <memory>
#include <functional>

namespace lrit
{
    class LRITDemux
    {
    private:
        const int d_mpdu_size;
        const bool d_check_crc;

    private:
        std::map<int, std::unique_ptr<ccsds::ccsds_weather::Demuxer>> demuxers;
        std::map<int, std::map<int, LRITFile>> wip_files;

        std::vector<LRITFile> files;

    private:
        void processLRITHeader(LRITFile &file, ccsds::CCSDSPacket &pkt);
        void parseHeader(LRITFile &file);
        void processLRITData(LRITFile &file, ccsds::CCSDSPacket &pkt);
        void finalizeLRITData(LRITFile &file);

    public:
        std::function<void(LRITFile &)> onParseHeader =
            [](LRITFile &) -> void {};
        std::function<bool(LRITFile &, ccsds::CCSDSPacket &)> onProcessData =
            [](LRITFile &, ccsds::CCSDSPacket &) -> bool
        { return true; };
        std::function<void(LRITFile&)> onFinalizeData =
            [](LRITFile&) -> void {};

    public:
        LRITDemux(int mpdu_size = 884, bool check_crc = true);
        ~LRITDemux();

        std::vector<LRITFile> work(uint8_t *cadu);
    };
};