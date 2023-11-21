#pragma once

#define NUM_T_RC_VALUES 5
#define NUM_BB_THERMISTORS 12

// Structure which holds engineering coefficients
typedef struct
{
    float Rp;
    float V0_CPA;
    float I_CPA;
    float V0_CPB;
    float I_CPB;

    float C_FP1[6];
    float C_FP2[6];
    float C_FP3[3][6];
    float C_FP4[3][6];
    float C_MIR_A[6];
    float C_MIR_B[6];
    float C_CAV_A[6];
    float C_CAV_B[6];
    float C_ADC[6];
    float C_INS_A[6];
    float C_INS_B[6];
    float C_RC[NUM_T_RC_VALUES][6];
    float C_RC_VR_FPA[6];
    float BB_a[NUM_BB_THERMISTORS][4];
} Engineering_Coefficients_t;

/*   ENGINEERING COEFFICIENTS FOR TERRA (PFM)
 *
 * -------------------------------------------------------
 *   DN to Eng Units equation coefficients
 *   Reference: SBRS Doc. 151840 REV C, T20-5A and T20-6A
 * -------------------------------------------------------
 */
Engineering_Coefficients_t Engineering_Coefficients_PFM = {
    2.0000E+07,  /* Rp     */
    -3.216341,   /* V0_CPA */
    0.000332142, /* I_CPA */
    -3.215385,   /* V0_CPB */
    0.000332376, /* I_CPB */

    /*
     * T_fp1 --> TA_AO_VIS_FPA
     * T_fp2 --> TA_AO_NIR_FPA
     * unit = ^oC
     * Ref. T20-5A    SBRS Doc. 151840 REV C
     */
    {/* TERRA (PFM) C_FP1[6] */
     -5.8957e+01, 3.5611e-02, 2.7192e-06, -1.2548e-09, 1.4924e-13, 0.},
    {/* TERRA (PFM) C_FP2[6]*/
     -5.7755e+01, 3.5526e-02, 2.8295e-06, -1.2865e-09, 1.5373e-13, 0.},

    /*
     * T_fp3 --> TA_RC_SMIR_CFPA
     * T_fp4 --> TA_RC_LWIR_CFPA
     * unit = K
     * Ref. T20-5A    SBRS Doc. 151840 REV C
     */
    {/*[FP_SPSs][6]*/ /*TA_RC_SMIR_CFPA*/ /* TERRA (PFM) C_FP3[3][6] */
     {4.9970e+01, 1.8360e-02, -9.1174e-07, 1.7673e-10, -1.3356e-14, 0.},
     {4.9912e+01, 1.8426e-02, -9.6036e-07, 1.9249e-10, -1.5208e-14, 0.},
     {4.9846e+01, 1.8413e-02, -9.4652e-07, 1.8741e-10, -1.4575e-14, 0.}},

    {/*[FP_SPSs][6]*/ /*TA_RC_LWIR_CFPA*/ /* TERRA (PFM) C_FP4[3][6] */
     {8.0062e+01, 1.4493e-03, -6.8684e-09, 1.9367e-12, -2.5410e-16, 0.},
     {8.2050e+01, 1.4442e-03, -5.6085e-09, 1.0498e-12, -1.0151e-16, 0.},
     {8.5048e+01, 1.4388e-03, -8.8412e-09, 2.5727e-12, -2.9433e-16, 0.}},

    /*
     * T_mir --> TP_SA_RCT1_MIR & TP_SA_RCT2_MIR,
     * they use the same coefficients
     * unit = ^oC
     * Ref. T20-6A    SBRS Doc. 151840 REV C
     */

    /* Common A/B side for Terra (PFM), Separate A/B sides for Aqua (FM1). */
    {/* TERRA (PFM) C_MIR_A[6] */
     8.6953e+01, -7.0286e-02, 4.1191e-05, -1.6989e-08,
     3.6372e-12, -3.2037e-16},
    {/* TERRA (PFM) C_MIR_B[6] */
     8.6953e+01, -7.0286e-02, 4.1191e-05, -1.6989e-08,
     3.6372e-12, -3.2037e-16},

    /*
     *   T_cav --> TP_MF_CALBKHD_SR, TP_SA_A_MTR
     *   unit = ^oC
     *   Ref. T20-6
     */

    /* Common A/B side for Terra (PFM), Separate A/B sides for Aqua (FM1). */
    {/* TERRA (PFM) C_CAV_A[6] */
     8.6954e+01, -5.6241e-01, 2.6375e-03, -8.7030e-06,
     1.4904e-08, -1.0501e-11},
    {/* TERRA (PFM) C_CAV_B[6] */
     8.6954e+01, -5.6241e-01, 2.6375e-03, -8.7030e-06,
     1.4904e-08, -1.0501e-11},

    /*
     *  T_ADC --> TA_PVLW_PWB4_10
     *              TA_PVSM_PWB6_12
     *   unit = ^oC
     *   Ref. T20-5A    SBRS Doc. 151840 REV C
     */
    {/* TERRA (PFM) C_ADC[6] */
     -1.8888e+01, 4.9724e-01, -2.3834e-03, 8.1515e-06,
     -1.3848e-08, 9.2793e-12},
    /*
     *   T_INS --> TP_AO_SMIR_OBJ, TP_AO_SMIR_LENS,
     *             TP_AO_LWIR_OBJ, TP_AO_LWIR_LENS
     *   unit = ^oC
     *   Ref. T20-6A    SBRS Doc. 151840 REV C
     */

    /* Common A/B side for Terra (PFM), Separate A/B sides for Aqua (FM1). */
    {/* TERRA (PFM) C_INS_A[6] */
     8.6953e+01, -7.0286e-02, 4.1191e-05, -1.6989e-08,
     3.6372e-12, -3.2037e-16},
    {/* TERRA (PFM) C_INS_B[6] */
     8.6953e+01, -7.0286e-02, 4.1191e-05, -1.6989e-08,
     3.6372e-12, -3.2037e-16},

    /* RC temperature conversion
     * num bits = 12 (max DN value = 4095)
     * units = Kelvin
     * ref. T20-5A    SBRS Doc. 151840 REV C
     */
    {
        /* TERRA (PFM) C_RC[NUM_T_RC_VALUES][6] */
        {-2.6607e+00, 2.9239e-02, -1.2789e-06, 1.4882e-10, 0., 0.}, /* TA_RC_CS */
        {-1.3726e+02, 7.5134e-02, -2.7944e-06, 3.5353e-09, 0., 0.}, /* TA_RC_CS_OG */
        {3.1238e+01, 2.6812e-02, 1.8002e-06, 2.8576e-11, 0., 0.},   /* TA_RC_IS */
        {-1.4316e+02, 8.0885e-02, -4.5854e-06, 3.7057e-09, 0., 0.}, /* TA_RC_IS_OG */
        {-1.4102e+02, 7.9490e-02, -4.3164e-06, 3.6498e-09, 0., 0.}  /* TA_RC_OS_OG */
    },

    /* RC FPA voltage heater conversion (LWIR or SMIR)
     * num bits = 10 (max DN value = 1023)
     * units = volts DC
     *    VR_RC_LW_FPA_HTR
     *    VR_RC_SM_FPA_HTR
     * ref T20-5    SBRS Doc. 151840 REV C
     */
    {/* TERRA (PFM) C_RC_VR_FPA[6] */
     -1.4970e+01, 2.9238e-02, 0., 0., 0., 0.},

    /*
     * All engineering coefficients for Terra OBC BB are described in
     * "Preliminary Investigation of the On-Board Calibrator (OBC) Blackbody
     * Temperature Calibration Algorithm" by J. Xiong and T. Dorman,
     * 03/24/1998.
     */
    {/* TERRA (PFM) BB_a[NUM_BB_THERMISTORS][4] */
     {9.298819E-04, 3.028024E-04, -4.973001E-06, 3.284554E-07},
     {9.521969E-04, 2.972846E-04, -4.348669E-06, 3.039369E-07},
     {9.614388E-04, 2.905839E-04, -3.482982E-06, 2.769668E-07},
     {9.533260E-04, 2.949576E-04, -4.035258E-06, 2.947707E-07},
     {9.946773E-04, 2.814390E-04, -2.494828E-06, 2.345659E-07},
     {9.808668E-04, 2.872997E-04, -3.192201E-06, 2.592090E-07},
     {9.699586E-04, 2.888108E-04, -3.350490E-06, 2.674701E-07},
     {9.550755E-04, 2.944409E-04, -3.996653E-06, 2.917877E-07},
     {9.491089E-04, 2.977076E-04, -4.445135E-06, 3.096761E-07},
     {1.011649E-03, 2.766499E-04, -1.983597E-06, 2.138617E-07},
     {1.028646E-03, 2.700306E-04, -1.171597E-06, 1.830096E-07},
     {1.001408E-03, 2.788388E-04, -2.210990E-06, 2.223562E-07}}};

/*   ENGINEERING COEFFICIENTS FOR AQUA (FM1)
 *
 * -------------------------------------------------------
 *   DN to Eng Units equation coefficients
 *   Reference: SBRS Doc. 151840 REV C, T20-5B and T20-6B
 * -------------------------------------------------------
 */
Engineering_Coefficients_t Engineering_Coefficients_FM1 = {
    2.0000E+07,  /* Rp     */
    -3.225313,   /* V0_CPA */
    0.000333193, /* I_CPA */
    -3.228853,   /* V0_CPB */
    0.000334205, /* I_CPB */

    /*
     * T_fp1 --> TA_AO_VIS_FPA
     * T_fp2 --> TA_AO_NIR_FPA
     * unit = ^oC
     * Ref. T20-5B    SBRS Doc. 151840 REV C
     */
    {/* AQUA (FM1) C_FP1[6] */
     -5.8639e+01, 3.6188e-02, 2.6495e-06, -1.3749e-09, 1.7933e-13, 0.},
    {/* AQUA (FM1) C_FP2[6] */
     -5.8907e+01, 3.5310e-02, 2.8666e-06, -1.3021e-09, 1.5744e-13, 0.},

    /*
     * T_fp3 --> TA_RC_SMIR_CFPA
     * T_fp4 --> TA_RC_LWIR_CFPA
     * unit = K
     * Ref. T20-5B    SBRS Doc. 151840 REV C
     */
    {/*[FP_SPSs][6]*/ /*TA_RC_SMIR_CFPA*/ /* AQUA (FM1) C_FP3[3][6] */
     {4.9904e+01, 1.8157e-02, -6.8273e-07, 1.0882e-10, -7.0948e-15, 0.},
     {4.9832e+01, 1.8200e-02, -7.0521e-07, 1.1395e-10, -7.5264e-15, 0.},
     {4.9762e+01, 1.8208e-02, -7.0835e-07, 1.1460e-10, -7.5779e-15, 0}},

    {/*[FP_SPSs][6]*/ /*TA_RC_LWIR_CFPA*/ /* AQUA (FM1) C_FP4[3][6] */
     {8.0049e+01, 1.4429e-03, -2.0639e-09, 2.0749e-14, 0., 0.},
     {8.2079e+01, 1.4374e-03, -2.0972e-09, 3.9308e-14, 0., 0.},
     {8.5062e+01, 1.4287e-03, -1.8224e-09, 9.6394e-14, 0., 0}},

    /*
     * T_mir --> TP_SA_RCT1_MIR & TP_SA_RCT2_MIR,
     * they use the same coefficients
     * unit = ^oC
     * Ref. T20-6B    SBRS Doc. 151840 REV C
     */

    /* Common A/B side for Terra (PFM), Separate A/B sides for Aqua (FM1). */
    {/* AQUA (FM1) C_MIR_A[6] */
     8.6330e+01, -6.9256e-02, 4.0275e-05, -1.6550e-08,
     3.5331e-12, -3.1087e-16},
    {/* AQUA (FM1) C_MIR_B[6] */
     8.7354E+01, -7.0526E-02, 4.1313E-05, -1.6883E-08,
     3.5664E-12, -3.0902E-16},
    /*
     *   T_cav --> TP_MF_CALBKHD_SR, TP_SA_A_MTR
     *   unit = ^oC
     *   Ref. T20-6
     */

    /* Common A/B side for Terra (PFM), Separate A/B sides for Aqua (FM1). */
    {/* AQUA (FM1) C_CAV_A[6] */
     8.6334e+01, -5.5420e-01, 2.5791e-03, -8.4794e-06,
     1.4482e-08, -1.0193e-11},
    {/* AQUA (FM1) C_CAV_B[6] */
     8.7352e+01, -5.6419e-01, 2.6441e-03, -8.6454e-06,
     1.4614e-08, -1.0132e-11},

    /*
     *  T_ADC --> TA_PVLW_PWB4_10
     *              TA_PVSM_PWB6_12
     *   unit = ^oC
     *   Ref. T20-5B    SBRS Doc. 151840 REV C
     */

    {/* AQUA (FM1) C_ADC[6] */
     -1.8888e+01, 4.9724e-01, -2.3834e-03, 8.1515e-06,
     -1.3848e-08, 9.2793e-12},
    /*
     *   T_INS --> TP_AO_SMIR_OBJ, TP_AO_SMIR_LENS,
     *             TP_AO_LWIR_OBJ, TP_AO_LWIR_LENS
     *   unit = ^oC
     *   Ref. T20-6B    SBRS Doc. 151840 REV C
     */

    /* Common A/B side for Terra (PFM), Separate A/B sides for Aqua (FM1). */
    {/* AQUA (FM1) C_INS_A[6] */
     8.6330e+01, -6.9256e-02, 4.0275e-05, -1.6550e-08,
     3.5331e-12, -3.1087e-16},
    {/* AQUA (FM1) C_INS_B[6] */
     8.7354e+01, -7.0526e-02, 4.1313e-05, -1.6883e-08,
     3.5664e-12, -3.0902e-16},

    /* RC temperature conversion
     * num bits = 12 (max DN value = 4095)
     * units = Kelvin
     *    VR_RC_LW_FPA_HTR
     *    VR_RC_SM_FPA_HTR
     * ref. T20-5B    SBRS Doc. 151840 REV C
     */

    {
        /* AQUA (FM1) C_RC[NUM_T_RC_VALUES][6] */
        {-2.6607e+00, 2.9239e-02, -1.2789e-06, 1.4882e-10, 0., 0.}, /* TA_RC_CS */
        {-2.0315e+02, 1.3710e-01, -2.1847e-05, 5.4693e-09, 0., 0.}, /* TA_RC_CS_OG */
        {3.1238e+01, 2.6812e-02, 1.8002e-06, 2.8576e-11, 0., 0.},   /* TA_RC_IS */
        {-2.0199e+02, 1.3610e-01, -2.1620e-05, 5.4342e-09, 0., 0.}, /* TA_RC_IS_OG */
        {-2.0916e+02, 1.4514e-01, -2.5230e-05, 5.8560e-09, 0., 0.}  /* TA_RC_OS_OG */
    },

    /* RC FPA voltage heater conversion (LWIR or SMIR)
     * num bits = 10 (max DN value = 1023)
     * units = volts DC
     * ref T20-5B    SBRS Doc. 151840 REV C
     */
    {/* AQUA (FM1) C_RC_VR_FPA[6] */
     -1.4970e+01, 2.9238e-02, 0., 0., 0., 0.},

    {/* AQUA (FM1) BB_a[NUM_BB_THERMISTORS][4] */
     {1.056236E-03, 2.599020E-04, -5.425454E-08, 1.415182E-07},
     {1.044243E-03, 2.644832E-04, -5.928912E-07, 1.628909E-07},
     {1.047991E-03, 2.626818E-04, -3.507229E-07, 1.533612E-07},
     {1.023606E-03, 2.710050E-04, -1.348365E-06, 1.935789E-07},
     {9.557013E-04, 2.970908E-04, -4.471715E-06, 3.073990E-07},
     {9.921567E-04, 2.816634E-04, -2.570464E-06, 2.376100E-07},
     {9.778825E-04, 2.874839E-04, -3.200168E-06, 2.605131E-07},
     {9.724649E-04, 2.883692E-04, -3.300496E-06, 2.659593E-07},
     {1.061567E-03, 2.579380E-04, 1.435982E-07, 1.358619E-07},
     {1.048034E-03, 2.628100E-04, -3.993544E-07, 1.539620E-07},
     {1.103644E-03, 2.444421E-04, 1.686106E-06, 7.536176E-08},
     {1.083254E-03, 2.507835E-04, 9.546836E-07, 1.038510E-07}}};