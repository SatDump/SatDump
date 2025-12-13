#pragma once

#include "tool.h"

namespace satdump
{
    class CCSDSAPIDDemuxTool : public BitViewTool
    {
    private:
        bool should_process = false;

        bool aos_mode = true;
        int mpdu_data_size = 884;
        int insert_zone_size = 0;
        bool split_apid = true;
        int cadu_size_bytes = 1024;

    public:
        std::string getName();
        void renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy);
        bool needToProcess();
        void setProcessed();
        void process(std::shared_ptr<BitContainer> &container, float &process_progress);
    };
} // namespace satdump