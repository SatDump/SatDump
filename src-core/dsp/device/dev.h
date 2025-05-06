#pragma once

#include "dsp/block.h"
#include <cstdint>

// TODOREWORK DOCUMENT
namespace satdump
{
    namespace ndsp
    {
        struct DeviceInfo
        {
            std::string type;      // SDR Type ID string
            std::string name;      // Display name
            nlohmann::json cfg;    // Default parameters for the block to use THIS device (eg, Serial). Applied with set_cfg
            nlohmann::json params; // List of additional parameters that may be device-specific (eg, Airspy Mini vs Airspy R2)

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(DeviceInfo, type, name, cfg, params)
        };

        class DeviceBlock : public Block
        {
        private:
            bool work() { throw satdump_exception("Device Source and/or Sync does not implement work. Should NOT have been called!"); }

        public:
            // TODOREWORK?
            enum device_mode_t
            {
                MODE_NORMAL = 0,
                MODE_SINGLE_RX = 1,
                MODE_SINGLE_TX = 2,
            };

        protected:
            DeviceInfo devInfo;
            device_mode_t devMode = MODE_NORMAL;

        public:
            DeviceBlock(std::string id, std::vector<BlockIO> in = {}, std::vector<BlockIO> out = {}) : Block(id, in, out) {}

            virtual void setDevInfo(DeviceInfo i, device_mode_t m)
            {
                devInfo = i, devMode = m;
                set_cfg(devInfo.cfg);
            }

            // TODOREWORK helper functions for basic functions
            virtual double getStreamSamplerate(int id, bool output) = 0;
            virtual void setStreamSamplerate(int id, bool output, double samplerate) = 0;
            virtual double getStreamFrequency(int id, bool output) = 0;
            virtual void setStreamFrequency(int id, bool output, double frequency) = 0;
        };

        struct RequestDeviceListEvent
        {
            std::vector<DeviceInfo> &i;
            DeviceBlock::device_mode_t m;
        };

        std::vector<DeviceInfo> getDeviceList(DeviceBlock::device_mode_t m);

        struct RequestDeviceInstanceEvent
        {
            DeviceInfo info;
            std::vector<std::shared_ptr<DeviceBlock>> &i;
        };

        std::shared_ptr<DeviceBlock> getDeviceInstanceFromInfo(DeviceInfo i, DeviceBlock::device_mode_t m);
    } // namespace ndsp
} // namespace satdump