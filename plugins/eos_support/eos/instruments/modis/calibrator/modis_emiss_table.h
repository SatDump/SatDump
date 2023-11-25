#pragma once

// TODO : Convert from HDF to CBOR, have a proper structure!u

#define NUM_EMISSIVE_DETECTORS 160
#define MAX_NUM_RSR_vs_LAMBDA_AQUA 66
#define MAX_NUM_RSR_vs_LAMBDA_TERRA 49
#define NUM_a0_vs_T_inst_COEFF 3
#define NUM_a2_vs_T_inst_COEFF 3
#define NUM_MIRROR_SIDES 2
#define NUM_EMISSIVE_BANDS 16
#define DETECTORS_PER_1KM_BAND 10
#define NUM_2ND_ORDER_COEFFS 3
#define NUM_PC_XT_BANDS 5
#define NUM_PC_XT_PARAMETERS 4

struct Coefficients_Emissive
{
    float epsilon_bb[NUM_EMISSIVE_DETECTORS];
    float epsilon_cav[NUM_EMISSIVE_DETECTORS];
    float delta_T_bb_beta[NUM_EMISSIVE_DETECTORS];
    float delta_T_bb_delta[NUM_EMISSIVE_DETECTORS];

    float RSR_AQUA[NUM_EMISSIVE_DETECTORS][MAX_NUM_RSR_vs_LAMBDA_AQUA];
    float WAVELENGTH_AQUA[NUM_EMISSIVE_DETECTORS][MAX_NUM_RSR_vs_LAMBDA_AQUA];
    float RSR_TERRA[NUM_EMISSIVE_DETECTORS][MAX_NUM_RSR_vs_LAMBDA_TERRA];
    float WAVELENGTH_TERRA[NUM_EMISSIVE_DETECTORS][MAX_NUM_RSR_vs_LAMBDA_TERRA];

    float A0[NUM_a0_vs_T_inst_COEFF][NUM_MIRROR_SIDES][NUM_EMISSIVE_DETECTORS];
    float A2[NUM_a2_vs_T_inst_COEFF][NUM_MIRROR_SIDES][NUM_EMISSIVE_DETECTORS];

    float RVS_TEB[NUM_EMISSIVE_BANDS][DETECTORS_PER_1KM_BAND][NUM_MIRROR_SIDES][NUM_2ND_ORDER_COEFFS];
    short RVS_BB_SV_Frame_No[2];

    float sigma_RVS_EV[NUM_EMISSIVE_BANDS][DETECTORS_PER_1KM_BAND][NUM_MIRROR_SIDES][NUM_2ND_ORDER_COEFFS];

    short NUM_RSR_vs_Lambda[NUM_EMISSIVE_DETECTORS];

    // char PCX_correction_switch[1];
    // float PC_XT[NUM_PC_XT_BANDS][DETECTORS_PER_1KM_BAND][NUM_PC_XT_PARAMETERS];
};

inline void from_json(const nlohmann::json &j, Coefficients_Emissive &v)
{
    memcpy(v.epsilon_bb, j["epsilon_bb"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.epsilon_cav, j["epsilon_cav"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.delta_T_bb_beta, j["delta_T_bb_beta"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.delta_T_bb_delta, j["delta_T_bb_delta"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));

    memcpy(v.RSR_AQUA, j["RSR"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * MAX_NUM_RSR_vs_LAMBDA_AQUA * sizeof(float));
    memcpy(v.WAVELENGTH_AQUA, j["WAVELENGTH"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * MAX_NUM_RSR_vs_LAMBDA_AQUA * sizeof(float));
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    memcpy(v.RSR_TERRA, j["RSR"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * MAX_NUM_RSR_vs_LAMBDA_TERRA * sizeof(float));
    memcpy(v.WAVELENGTH_TERRA, j["WAVELENGTH"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * MAX_NUM_RSR_vs_LAMBDA_TERRA * sizeof(float));

    memcpy(v.A0, j["A0"].get<std::vector<float>>().data(), NUM_a0_vs_T_inst_COEFF * NUM_MIRROR_SIDES * NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.A2, j["A2"].get<std::vector<float>>().data(), NUM_a2_vs_T_inst_COEFF * NUM_MIRROR_SIDES * NUM_EMISSIVE_DETECTORS * sizeof(float));

    memcpy(v.RVS_TEB, j["RVS_TEB"].get<std::vector<float>>().data(), NUM_EMISSIVE_BANDS * DETECTORS_PER_1KM_BAND * NUM_MIRROR_SIDES * NUM_2ND_ORDER_COEFFS * sizeof(float));
    memcpy(v.RVS_BB_SV_Frame_No, j["RVS_BB_SV_Frame_No"].get<std::vector<short>>().data(), 2 * sizeof(short));

    memcpy(v.sigma_RVS_EV, j["sigma_RVS_EV"].get<std::vector<float>>().data(), NUM_EMISSIVE_BANDS * DETECTORS_PER_1KM_BAND * NUM_MIRROR_SIDES * NUM_2ND_ORDER_COEFFS * sizeof(float));

    memcpy(v.NUM_RSR_vs_Lambda, j["NWL"].get<std::vector<short>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(short));

    // memcpy(v.PCX_correction_switch, j["PCX_correction_switch"].get<std::vector<char>>().data(), 1 * sizeof(char));
    // memcpy(v.PC_XT, j["PC_XT"].get<std::vector<float>>().data(), NUM_PC_XT_BANDS * DETECTORS_PER_1KM_BAND * NUM_PC_XT_PARAMETERS * sizeof(char));
}