#pragma once

#include "base/remote_handler.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace satdump
{
    namespace handlers
    {
        class TestRemoteHandlerHandler : public RemoteHandlerHandler
        {
        private:
            double val;

        protected:
            bool mustUpdate = true;
            void handle_stream_data(std::string id, uint8_t *data, size_t size)
            {
                if (id == "upd")
                    mustUpdate = true;
            }

        public:
            TestRemoteHandlerHandler(std::shared_ptr<RemoteHandlerBackend> bkd) : RemoteHandlerHandler(bkd) {}
            ~TestRemoteHandlerHandler() {}

            // The Rest
            void drawMenu()
            {
                if (mustUpdate)
                {
                    ImGui::ClearActiveID();

                    val = bkd->get_cfg("val");

                    mustUpdate = false;
                }

                if (ImGui::InputDouble("Val", &val))
                    bkd->set_cfg("val", val);
            }
            void drawContents(ImVec2 win_size) {}

            std::string getName() { return "RemoteHandleTest"; }

            std::string getID() { return "remote_test_handler"; }
        };
    } // namespace handlers
} // namespace satdump