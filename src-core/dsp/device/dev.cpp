#include "dev.h"
#include "core/plugin.h"
#include "logger.h"

namespace satdump
{
    namespace ndsp
    {
        std::vector<DeviceInfo> getDeviceList()
        {
            std::vector<DeviceInfo> l;

            eventBus->fire_event<RequestDeviceListEvent>({l});

            logger->trace("Device list :");
            for (auto &d : l)
                logger->trace(" - " + d.name);

            return l;
        }

        std::shared_ptr<DeviceBlock> getDeviceInstanceFromInfo(DeviceInfo i)
        {
            std::vector<std::shared_ptr<DeviceBlock>> l;

            eventBus->fire_event<RequestDeviceInstanceEvent>({i, l});

            if (l.size() == 0)
                throw satdump_exception("Could not get device instance! " + i.name);
            else if (l.size() > 1)
                throw satdump_exception("Got more than one device instance! " + i.name);

            for (auto &li : l)
                li->setDevInfo(i);

            return l[0];
        }
    } // namespace ndsp
} // namespace satdump