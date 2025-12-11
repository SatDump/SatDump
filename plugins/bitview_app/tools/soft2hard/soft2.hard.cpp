#include "core/style.h"
#include "logger.h"
#include "soft2hard.h"
#include <fstream>

namespace satdump
{
    std::string Soft2HardTool::getName() { return "Soft To Hard"; }

    void Soft2HardTool::renderMenu(std::shared_ptr<BitContainer> &container, bool is_busy)
    {
        if (is_busy)
            style::beginDisabled();

        if (ImGui::Button("Soft To Hard"))
            should_process = true;

        if (is_busy)
            style::endDisabled();
    }

    bool Soft2HardTool::needToProcess() { return should_process; }

    void Soft2HardTool::setProcessed() { should_process = false; }

    void Soft2HardTool::process(std::shared_ptr<BitContainer> &container, float &process_progress)
    {
        uint8_t *ptr = container->get_ptr();
        size_t size = container->get_ptr_size();

        char name[1000];
        tmpnam(name);
        std::string tmpfile = name;
        std::ofstream file_out(tmpfile, std::ios::binary);

        int bitpos = 0;
        uint8_t tmp_buf = 0;

        size_t current_ptr = 0;
        while (current_ptr < size)
        {
            size_t csize = std::min<size_t>(8192, size - current_ptr);

            int8_t *ptr_pos = (int8_t *)ptr + current_ptr;

            for (int i = 0; i < csize; i++)
            {
                tmp_buf = tmp_buf << 1 | (ptr_pos[i] >= 0);
                bitpos++;

                if (bitpos == 8)
                {
                    file_out.write((char *)&tmp_buf, 1);
                    bitpos = 0;
                }
            }

            current_ptr += csize;

            process_progress = double(current_ptr) / double(size);
        }

        file_out.close();

        std::shared_ptr<satdump::BitContainer> newbitc = std::make_shared<satdump::BitContainer>(container->getName() + " Hard", tmpfile);
        newbitc->d_bitperiod = container->d_bitperiod / 8;
        newbitc->init_bitperiod();
        newbitc->d_is_temporary = true;

        if (container->bitview != nullptr)
            ((BitViewHandler *)container->bitview)->addSubHandler(std::make_shared<BitViewHandler>(newbitc));
        else
            logger->error("Can't add container!");
    }
} // namespace satdump