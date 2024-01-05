#pragma once

#include "nlohmann/json.hpp"

namespace jpss
{
    namespace atms
    {
        const int NUM_PRT_KAV = 8;
        const int NUM_PRT_WG = 7;
        const int NUM_ATMS_BANDS = 5;
        const int NUM_CHANNELS = 22;
        const int NUM_PRT_COEFFS = 4;
        const int NUM_PRT_COEFF_2WIRE = 56;
        const int NUM_HOUSE_KEEPING = 4;

        const int ATTITUDE = 3;
        const int BEAM_POS_OFFSET = 3;
        const int MIN_MAX_DIMENSION = 2;
        const int NUM_BAND_CATEGORIES = 2;
        const int NUM_BC_COEFFS = 2;
        const int NUM_BEAM_POSITIONS = 96;
        const int NUM_BIAS_COEFFS = 3;
        const int NUM_COLD_PLATE_TEMP = 3;
        const int NUM_COLD_SAMPLES = 4;
        const int NUM_COLD_SCAN_PROFILES = 4;
        const int NUM_COLD_SMAPLES = 4;
        const int NUM_HS_VARS = 74;
        const int NUM_MAP_RC_SIZE = 8;
        const int NUM_NL_COEFFS = 3;
        const int NUM_PADBYTES_COEFF = 1;
        const int NUM_REDUNDANCY_CONFIGS = 4;
        const int NUM_SCAN_CC = 10;
        const int NUM_SCAN_PRT = 9;
        const int NUM_SCAN_WC = 10;
        const int NUM_SHELF_TEMPS = 4;
        const int NUM_WARM_SAMPLES = 4;
        const int NUM_WARM_SCAN_PROFILES = 4;

        struct ATMSCalibPkt
        {
            double pamKav;
            double pamWg;
            double prtCoeffKav[NUM_PRT_KAV][NUM_PRT_COEFFS]; // R0, Alpha, Delta, Beta
            double prtCoeffWg[NUM_PRT_WG][NUM_PRT_COEFFS];   // R0, Alpha, Delta, Beta
            double warmBias[NUM_ATMS_BANDS];
            double coldBias[NUM_ATMS_BANDS];
            double quadraticCoeffs[NUM_CHANNELS];
            // double beamAlignment // Not needed
            double prtCoeffShelf[NUM_PRT_COEFFS][NUM_PRT_COEFFS]; // R0, Alpha, Delta, Rc
            double prtCoeff2Wire[NUM_PRT_COEFF_2WIRE];            // R0, A1
            double houseKeeping[NUM_HOUSE_KEEPING];
            bool valid = false;
        };

        inline void to_json(nlohmann::json &j, const ATMSCalibPkt &v)
        {
            j["pamKav"] = v.pamKav;
            j["pamWg"] = v.pamWg;
            for (int i = 0; i < NUM_PRT_KAV; i++)
                for (int y = 0; y < NUM_PRT_COEFFS; y++)
                    j["prtCoeffKav"][i][y] = v.prtCoeffKav[i][y];
            for (int i = 0; i < NUM_PRT_WG; i++)
                for (int y = 0; y < NUM_PRT_COEFFS; y++)
                    j["prtCoeffWg"][i][y] = v.prtCoeffWg[i][y];
            for (int i = 0; i < NUM_ATMS_BANDS; i++)
                j["warmBias"][i] = v.warmBias[i];
            for (int i = 0; i < NUM_ATMS_BANDS; i++)
                j["coldBias"][i] = v.coldBias[i];
            for (int i = 0; i < NUM_CHANNELS; i++)
                j["quadraticCoeffs"][i] = v.quadraticCoeffs[i];
            for (int i = 0; i < NUM_PRT_COEFFS; i++)
                for (int y = 0; y < NUM_PRT_COEFFS; y++)
                    j["prtCoeffShelf"][i][y] = v.prtCoeffShelf[i][y];
            for (int i = 0; i < NUM_PRT_COEFF_2WIRE; i++)
                j["prtCoeff2Wire"][i] = v.prtCoeff2Wire[i];
            for (int i = 0; i < NUM_HOUSE_KEEPING; i++)
                j["houseKeeping"][i] = v.houseKeeping[i];
            j["valid"] = v.valid;
        }

        inline void from_json(const nlohmann::json &j, ATMSCalibPkt &v)
        {
            v.pamKav = j["pamKav"];
            v.pamWg = j["pamWg"];
            for (int i = 0; i < NUM_PRT_KAV; i++)
                for (int y = 0; y < NUM_PRT_COEFFS; y++)
                    v.prtCoeffKav[i][y] = j["prtCoeffKav"][i][y];
            for (int i = 0; i < NUM_PRT_WG; i++)
                for (int y = 0; y < NUM_PRT_COEFFS; y++)
                    v.prtCoeffWg[i][y] = j["prtCoeffWg"][i][y];
            for (int i = 0; i < NUM_ATMS_BANDS; i++)
                v.warmBias[i] = j["warmBias"][i];
            for (int i = 0; i < NUM_ATMS_BANDS; i++)
                v.coldBias[i] = j["coldBias"][i];
            for (int i = 0; i < NUM_CHANNELS; i++)
                v.quadraticCoeffs[i] = j["quadraticCoeffs"][i];
            for (int i = 0; i < NUM_PRT_COEFFS; i++)
                for (int y = 0; y < NUM_PRT_COEFFS; y++)
                    v.prtCoeffShelf[i][y] = j["prtCoeffShelf"][i][y];
            for (int i = 0; i < NUM_PRT_COEFF_2WIRE; i++)
                v.prtCoeff2Wire[i] = j["prtCoeff2Wire"][i];
            for (int i = 0; i < NUM_HOUSE_KEEPING; i++)
                v.houseKeeping[i] = j["houseKeeping"][i];
            v.valid = j["valid"];
        }

        struct ATMSHealtStatusPkt
        {
            uint16_t data[NUM_HS_VARS];
            bool valid = false;
        };

        inline void to_json(nlohmann::json &j, const ATMSHealtStatusPkt &v)
        {
            for (int i = 0; i < NUM_HS_VARS; i++)
                j["data"][i] = v.data[i];
            j["valid"] = v.valid;
        }

        inline void from_json(const nlohmann::json &j, ATMSHealtStatusPkt &v)
        {
            for (int i = 0; i < NUM_HS_VARS; i++)
                v.data[i] = j["data"][i];
            v.valid = j["valid"];
        }

        struct ATMSHotCalTempPkt
        {
            uint16_t kavPrt[NUM_PRT_KAV];
            uint16_t kavPamCounts;
            uint16_t wqPrt[NUM_PRT_WG];
            uint16_t wgPamCounts;
            bool valid = false;
        };

        inline void to_json(nlohmann::json &j, const ATMSHotCalTempPkt &v)
        {
            for (int i = 0; i < NUM_PRT_KAV; i++)
                j["kavPrt"][i] = v.kavPrt[i];
            j["kavPamCounts"] = v.kavPamCounts;
            for (int i = 0; i < NUM_PRT_WG; i++)
                j["wqPrt"][i] = v.wqPrt[i];
            j["wgPamCounts"] = v.wgPamCounts;
            j["valid"] = v.valid;
        }

        inline void from_json(const nlohmann::json &j, ATMSHotCalTempPkt &v)
        {
            for (int i = 0; i < NUM_PRT_KAV; i++)
                v.kavPrt[i] = j["kavPrt"][i];
            v.kavPamCounts = j["kavPamCounts"];
            for (int i = 0; i < NUM_PRT_WG; i++)
                v.wqPrt[i] = j["wqPrt"][i];
            v.wgPamCounts = j["wgPamCounts"];
            v.valid = j["valid"];
        }

        struct ATMS_SDR_CC
        {
            double scanBias[NUM_CHANNELS][NUM_BEAM_POSITIONS];
            double beamEfficiencyCorrection[NUM_CHANNELS][NUM_BEAM_POSITIONS];
            double centralFrequency[NUM_CHANNELS];
            double radianceBandCorrection[NUM_BC_COEFFS][NUM_CHANNELS];
            float quadraticRc[NUM_NL_COEFFS][NUM_REDUNDANCY_CONFIGS][NUM_CHANNELS];
            float shelfTemp[NUM_COLD_PLATE_TEMP][NUM_SHELF_TEMPS];
            float prtConvergence;
            float reflectorEmissivity[NUM_CHANNELS];
            float reflectorTempOffset;
            int32_t mapRc[NUM_MAP_RC_SIZE];
            int32_t resolverOffset;
            int32_t prtLoops;
            int8_t useQuadraticTerm;
        };

        inline void from_json(const nlohmann::json &j, ATMS_SDR_CC &v)
        {
            for (int i = 0; i < NUM_CHANNELS; i++)
                for (int z = 0; z < NUM_BEAM_POSITIONS; z++)
                    v.scanBias[i][z] = j["scanBias"][i][z];
            for (int i = 0; i < NUM_CHANNELS; i++)
                for (int z = 0; z < NUM_BEAM_POSITIONS; z++)
                    v.beamEfficiencyCorrection[i][z] = j["beamEfficiencyCorrection"][i][z];
            for (int i = 0; i < NUM_CHANNELS; i++)
                v.centralFrequency[i] = j["centralFrequency"][i];
            for (int i = 0; i < NUM_BC_COEFFS; i++)
                for (int z = 0; z < NUM_CHANNELS; z++)
                    v.radianceBandCorrection[i][z] = j["radianceBandCorrection"][i][z];
            for (int i = 0; i < NUM_NL_COEFFS; i++)
                for (int z = 0; z < NUM_REDUNDANCY_CONFIGS; z++)
                    for (int k = 0; k < NUM_CHANNELS; k++)
                        v.quadraticRc[i][z][k] = j["quadraticRc"][i][z][k];
            for (int i = 0; i < NUM_COLD_PLATE_TEMP; i++)
                for (int z = 0; z < NUM_SHELF_TEMPS; z++)
                    v.shelfTemp[i][z] = j["shelfTemp"][i][z];
            v.prtConvergence = j["prtConvergence"];
            for (int i = 0; i < NUM_CHANNELS; i++)
                v.reflectorEmissivity[i] = j["reflectorEmissivity"][i];
            v.reflectorTempOffset = j["reflectorTempOffset"];
            for (int i = 0; i < NUM_MAP_RC_SIZE; i++)
                v.mapRc[i] = j["mapRc"][i];
            v.resolverOffset = j["resolverOffset"];
            v.prtLoops = j["prtLoops"];
            v.useQuadraticTerm = j["useQuadraticTerm"];
        }
    }

}
