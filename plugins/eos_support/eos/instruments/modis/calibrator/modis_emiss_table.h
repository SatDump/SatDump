#pragma once

// TODO : Convert from HDF to CBOR, have a proper structure!u

#define NUM_EMISSIVE_DETECTORS 160
#define NUM_PC_XT_BANDS 5
#define DETECTORS_PER_1KM_BAND 10
#define NUM_PC_XT_PARAMETERS 4
#define MAX_NUM_RSR_vs_LAMBDA 66
#define NTIMES_A0 18
#define NUM_a0_vs_T_inst_COEFF 3
#define NUM_MIRROR_SIDES 2
#define NTIMES_A2 18
#define NUM_a2_vs_T_inst_COEFF 3
// #define 1 1
#define NUM_T_INS_THERMISTORS 4
#define NUM_T_CAV_THERMISTORS 4
#define NUM_T_MIR_THERMISTORS 2
#define NUM_BB_THERMISTORS 12
#define NTIMES_RVS_TEB 1
#define NUM_EMISSIVE_BANDS 16
#define NUM_2ND_ORDER_COEFFS 3
// #define 2 2
#define NTIMES_Band_21_b1 34
#define NUM_AQUA_BB_SAT_BANDS 3
#define NTIMES_BB_T_sat_default_b1_baseline_aqua 50
#define NTIMES_BB_T_sat_default_b1_c1_aqua 39
#define NTIMES_BB_T_sat_default_b1_Tlwir_baseline_aqua 1
#define LWIR_FPA_Temperature 1
#define NTIMES_sigma_a0 25
#define NTIMES_sigma_a2 25
#define NTIMES_sigma_RVS_EV 1
#define NTIMES_sigma_epsilon_BB 1
#define NTIMES_sigma_epsilon_CAV 1
#define NTIMES_sigma_L_lambda 1
#define NUM_1ST_ORDER_COEFFS 2
#define NTIMES_sigma_L_Tbb 1
#define NTIMES_sigma_L_Tsm 1
#define NTIMES_sigma_L_Tcav 1
#define NTIMES_sigma_b1_B21 26
#define NTIMES_pcx_ui_factor 1

struct Coefficients_Emissive
{
    float epsilon_bb[NUM_EMISSIVE_DETECTORS];
    float epsilon_cav[NUM_EMISSIVE_DETECTORS];
    float delta_T_bb_beta[NUM_EMISSIVE_DETECTORS];
    float delta_T_bb_delta[NUM_EMISSIVE_DETECTORS];

    float RSR[NUM_EMISSIVE_DETECTORS][MAX_NUM_RSR_vs_LAMBDA];
    float WAVELENGTH[NUM_EMISSIVE_DETECTORS][MAX_NUM_RSR_vs_LAMBDA];

    float A0[NUM_a0_vs_T_inst_COEFF][NUM_MIRROR_SIDES][NUM_EMISSIVE_DETECTORS];
    float A2[NUM_a2_vs_T_inst_COEFF][NUM_MIRROR_SIDES][NUM_EMISSIVE_DETECTORS];

    float RVS_TEB[NUM_EMISSIVE_BANDS][DETECTORS_PER_1KM_BAND][NUM_MIRROR_SIDES][NUM_2ND_ORDER_COEFFS];
    short RVS_BB_SV_Frame_No[2];

    float sigma_RVS_EV[NUM_EMISSIVE_BANDS][DETECTORS_PER_1KM_BAND][NUM_MIRROR_SIDES][NUM_2ND_ORDER_COEFFS];

    // char PCX_correction_switch[1];
};

inline void from_json(const nlohmann::json &j, Coefficients_Emissive &v)
{
    memcpy(v.epsilon_bb, j["epsilon_bb"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.epsilon_cav, j["epsilon_cav"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.delta_T_bb_beta, j["delta_T_bb_beta"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.delta_T_bb_delta, j["delta_T_bb_delta"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * sizeof(float));

    memcpy(v.RSR, j["RSR"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * MAX_NUM_RSR_vs_LAMBDA * sizeof(float));
    memcpy(v.WAVELENGTH, j["WAVELENGTH"].get<std::vector<float>>().data(), NUM_EMISSIVE_DETECTORS * MAX_NUM_RSR_vs_LAMBDA * sizeof(float));

    memcpy(v.A0, j["A0"].get<std::vector<float>>().data(), NUM_a0_vs_T_inst_COEFF * NUM_MIRROR_SIDES * NUM_EMISSIVE_DETECTORS * sizeof(float));
    memcpy(v.A2, j["A2"].get<std::vector<float>>().data(), NUM_a2_vs_T_inst_COEFF * NUM_MIRROR_SIDES * NUM_EMISSIVE_DETECTORS * sizeof(float));

    memcpy(v.RVS_TEB, j["RVS_TEB"].get<std::vector<float>>().data(), NUM_EMISSIVE_BANDS * DETECTORS_PER_1KM_BAND * NUM_MIRROR_SIDES * NUM_2ND_ORDER_COEFFS * sizeof(float));
    memcpy(v.RVS_BB_SV_Frame_No, j["RVS_BB_SV_Frame_No"].get<std::vector<short>>().data(), 2 * sizeof(short));

    memcpy(v.sigma_RVS_EV, j["sigma_RVS_EV"].get<std::vector<float>>().data(), NUM_EMISSIVE_BANDS * DETECTORS_PER_1KM_BAND * NUM_MIRROR_SIDES * NUM_2ND_ORDER_COEFFS * sizeof(float));

    // memcpy(v.PCX_correction_switch, j["PCX_correction_switch"].get<std::vector<float>>().data(), 1 * sizeof(char));
}