#pragma once

#include "pipeline/modules/base/filestream_to_filestream.h"
#include "utils/mqtt_client.h"
#include <memory>

namespace elektro_arktika
{
    namespace ggak
    {
        class GGAKToMQTTModule : public satdump::pipeline::base::FileStreamToFileStreamModule
        {
        protected:
            std::unique_ptr<satdump::MQTTClient> mqtt;

        public:
            GGAKToMQTTModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            ~GGAKToMQTTModule();
            void process();
            void drawUI(bool window);
            nlohmann::json getModuleStats();

        public:
            static std::string getID();
            virtual std::string getIDM() { return getID(); };
            static nlohmann::json getParams() { return {}; } // TODOREWORK
            static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        };
    } // namespace ggak
} // namespace elektro_arktika