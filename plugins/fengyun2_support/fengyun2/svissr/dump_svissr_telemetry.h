#include "svissr_blocks.h"
#include <string>

namespace fengyun_svissr
{

    class SVISSRTelemetryDumper
    {

    public:
        /**
         * @brief Converts telemetry from arguments into a human-readable JSONC format
         *
         * @param OAT_block Orbit and attitude block data
         * @param Attitude_subblock Attitude prediction subblock data
         * @param Orbital_subblock Orbital prediction subblock data
         * @param active_sensor Primary/Backup sensor
         * @param active_detectors bites 8-1 showing whether sensor used is primary (1) or redundant (0)
         * @return std::string Formatted JSONC data
         */
        static std::string dump_telemetry_to_JSON(OrbitAndAttitudeData OAT_block, AttitudePredictionSubBlock Attitude_subblock, OrbitPredictionSubBlock Orbital_subblock, std::string active_sensor,
                                                  uint8_t active_detectors);
    };
} // namespace fengyun_svissr