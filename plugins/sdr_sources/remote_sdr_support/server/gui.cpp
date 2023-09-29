#include "gui.h"
#include "main.h"
#include "remote.h"

std::mutex gui_feedback_mtx;
RImGui::RImGui gui_local;
std::vector<RImGui::UiElem> last_draw_feedback;
uint64_t last_samplerate = 0;

void sourceGuiThread()
{
    RImGui::is_local = false;
    RImGui::current_instance = &gui_local;

    std::vector<uint8_t> buffer_tx;

    while (1)
    {
        source_mtx.lock();
        if (source_is_open)
        {
            gui_feedback_mtx.lock();
            if (last_draw_feedback.size() > 0)
            {
                // logger->info("FeedBack %d", last_draw_feedback.size());
                RImGui::set_feedback(last_draw_feedback);
                last_draw_feedback.clear();
            }
            gui_feedback_mtx.unlock();

            current_sample_source->drawControlUI();

            auto render_elems = RImGui::end_frame();

            if (render_elems.size() > 0)
            {
                // logger->info("DrawElems %d", render_elems.size());
                gui_feedback_mtx.lock();
                buffer_tx.resize(65535);
                int len = RImGui::encode_vec(buffer_tx.data(), render_elems);
                buffer_tx.resize(len);
                sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_GUI, buffer_tx);
                gui_feedback_mtx.unlock();
            }

            if (last_samplerate != current_sample_source->get_samplerate())
            {
                std::vector<uint8_t> pkt(8);
                *((uint64_t *)&pkt[0]) = last_samplerate = current_sample_source->get_samplerate();
                sendPacketWithVector(tcp_server, dsp::remote::PKT_TYPE_SAMPLERATEFBK, pkt);
            }
        }
        source_mtx.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}