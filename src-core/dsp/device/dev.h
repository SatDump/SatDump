#pragma once

#include "dsp/block.h"

// TODOREWORK DOCUMENT
namespace satdump
{
    namespace ndsp
    {
        struct DeviceInfo
        {
            std::string type;           // SDR Type ID string
            std::string name;           // Display name
            nlohmann::json cfg;         // Default parameters for the block to use THIS device
            nlohmann::json params = {}; // List of additional parameters that may be device-specific (eg, Airspy Mini vs Airspy R2)

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(DeviceInfo, type, name, cfg, params)
        };

        class DeviceBlock : public Block
        {
        private:
            bool work() { throw satdump_exception("Device Source and/or Sync does not implement work. Should NOT have been called!"); }

        protected:
            DeviceInfo devInfo;

        public:
            DeviceBlock(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : Block(id, in, out) {}
            virtual void drawUI() = 0;

            virtual void setDevInfo(DeviceInfo i) { devInfo = i; }
        };

        struct RequestDeviceListEvent
        {
            std::vector<DeviceInfo> &i;
        };

        std::vector<DeviceInfo> getDeviceList();

        struct RequestDeviceInstanceEvent
        {
            DeviceInfo info;
            std::vector<std::shared_ptr<DeviceBlock>> &i;
        };

        std::shared_ptr<DeviceBlock> getDeviceInstanceFromInfo(DeviceInfo i);
    }
}