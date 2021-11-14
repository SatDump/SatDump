#include "glm_parser.h"
#include "nlohmann/json.hpp"

namespace goes
{
    namespace grb
    {
        std::string parseGLMFrameToJSON(uint8_t *data, int size, products::GLM::GRBGLMProductType type)
        {
            nlohmann::ordered_json glm_data;

            if (type == products::GLM::FLASH)
            {
                uint64_t number_of_flashes = *((uint64_t *)&data[0]);
                glm_data["number_of_flashes"] = number_of_flashes;

                for (int i = 8; (i - 8) < number_of_flashes * 24 && i < size; i += 24)
                {
                    uint16_t flash_id = *((uint16_t *)&data[i + 0]);
                    uint16_t flash_time_offset_of_first_event = *((uint16_t *)&data[i + 2]);
                    uint16_t flash_time_offset_of_last_event = *((uint16_t *)&data[i + 4]);
                    uint16_t flash_frame_time_offset_of_first_event = *((uint16_t *)&data[i + 6]);
                    uint16_t flash_frame_time_offset_of_last_event = *((uint16_t *)&data[i + 8]);
                    float flash_lat = *((float *)&data[i + 10]);
                    float flash_lon = *((float *)&data[i + 14]);
                    uint16_t flash_area = *((uint16_t *)&data[i + 18]);
                    uint16_t flash_energy = *((uint16_t *)&data[i + 20]);
                    uint16_t flash_quality_flag = *((uint16_t *)&data[i + 22]);

                    glm_data["flashes"][i / 24]["flash_id"] = flash_id;
                    glm_data["flashes"][i / 24]["flash_time_offset_of_first_event"] = flash_time_offset_of_first_event;
                    glm_data["flashes"][i / 24]["flash_time_offset_of_last_event"] = flash_time_offset_of_last_event;
                    glm_data["flashes"][i / 24]["flash_frame_time_offset_of_first_event"] = flash_frame_time_offset_of_first_event;
                    glm_data["flashes"][i / 24]["flash_frame_time_offset_of_last_event"] = flash_frame_time_offset_of_last_event;
                    glm_data["flashes"][i / 24]["flash_lat"] = flash_lat;
                    glm_data["flashes"][i / 24]["flash_lon"] = flash_lon;
                    glm_data["flashes"][i / 24]["flash_area"] = flash_area;
                    glm_data["flashes"][i / 24]["flash_energy"] = flash_energy;
                    glm_data["flashes"][i / 24]["flash_quality_flag"] = flash_quality_flag;
                }
            }
            else if (type == products::GLM::GROUP) // Looks like the documentation is wrong. This is 24-bytes and not 28 !!!!
            {
                uint64_t number_of_groups = *((uint64_t *)&data[0]);
                glm_data["number_of_groups"] = number_of_groups;

                for (int i = 8; (i - 8) < number_of_groups * 24 && i < size; i += 24)
                {
                    uint32_t group_id = *((uint32_t *)&data[i + 0]);
                    uint16_t group_time_offset = *((uint16_t *)&data[i + 4]);
                    uint16_t group_frame_time_offset = *((uint16_t *)&data[i + 6]);
                    float group_lat = *((float *)&data[i + 8]);
                    float group_lon = *((float *)&data[i + 12]);
                    uint16_t group_area = *((uint16_t *)&data[i + 16]);
                    uint16_t group_energy = *((uint16_t *)&data[i + 18]);
                    uint16_t group_parent_flash_id = *((uint16_t *)&data[i + 20]);
                    uint16_t group_quality_flag = *((uint16_t *)&data[i + 22]);

                    glm_data["flashes"][i / 24]["group_id"] = group_id;
                    glm_data["flashes"][i / 24]["group_time_offset"] = group_time_offset;
                    glm_data["flashes"][i / 24]["group_frame_time_offset"] = group_frame_time_offset;
                    glm_data["flashes"][i / 24]["group_lat"] = group_lat;
                    glm_data["flashes"][i / 24]["group_lon"] = group_lon;
                    glm_data["flashes"][i / 24]["group_area"] = group_area;
                    glm_data["flashes"][i / 24]["group_energy"] = group_energy;
                    glm_data["flashes"][i / 24]["group_parent_flash_id"] = group_parent_flash_id;
                    glm_data["flashes"][i / 24]["group_quality_flag"] = group_quality_flag;
                }
            }
            else if (type == products::GLM::EVENT)
            {
                uint64_t number_of_events = *((uint64_t *)&data[0]);
                glm_data["number_of_events"] = number_of_events;

                for (int i = 8; (i - 8) < number_of_events * 16 && i < size; i += 16)
                {
                    uint32_t event_id = *((uint32_t *)&data[i + 0]);
                    uint16_t event_time_offset = *((uint16_t *)&data[i + 4]);
                    uint16_t event_lat = *((uint16_t *)&data[i + 6]);
                    uint16_t event_lon = *((uint16_t *)&data[i + 8]);
                    uint16_t event_energy = *((uint16_t *)&data[i + 10]);
                    uint32_t event_parent_group_id = *((uint32_t *)&data[i + 12]);

                    glm_data["flashes"][i / 16]["event_id"] = event_id;
                    glm_data["flashes"][i / 16]["event_time_offset"] = event_time_offset;
                    glm_data["flashes"][i / 16]["event_lat"] = event_lat;
                    glm_data["flashes"][i / 16]["event_lon"] = event_lon;
                    glm_data["flashes"][i / 16]["event_energy"] = event_energy;
                    glm_data["flashes"][i / 16]["event_parent_group_id"] = event_parent_group_id;
                }
            }

            return glm_data.dump(4);
        }
    }
}