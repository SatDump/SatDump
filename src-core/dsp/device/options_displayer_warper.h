#pragma once

#include "dsp/block.h"
#include "options_displayer.h"
#include <memory>

namespace satdump
{
    namespace ndsp
    {
        // TODOREWORK figure out how exactly this will be used longterm
        class OptDisplayerWarper
        {
        private:
            OptionsDisplayer disp;
            std::shared_ptr<Block> blk;

        public:
            OptDisplayerWarper(std::shared_ptr<Block> b) : blk(b)
            {
                disp.add_options(blk->get_cfg_list());
                disp.set_values(blk->get_cfg());
            }

            bool draw()
            {
                nlohmann::json changed = disp.draw();

                bool ret = false;

                if (changed.size() > 0)
                {
                    auto r = blk->set_cfg(changed);
                    if (r >= ndsp::Block::RES_LISTUPD)
                    {
                        disp.clear();
                        disp.add_options(blk->get_cfg_list());
                    }
                    ret = r >= ndsp::Block::RES_IOUPD;

                    disp.set_values(blk->get_cfg());
                }

                return ret;
            }

            void update()
            {
                disp.clear();
                disp.add_options(blk->get_cfg_list());
                disp.set_values(blk->get_cfg());
            }

            void setShowStats(bool v) { disp.show_stats = v; }
        };
    } // namespace ndsp
} // namespace satdump