
/*** Register 0: IC Mode / Power Control ***/

/* reg0: 4:8 (AM_MODE, VHF_MODE, B3_MODE, B45_MODE, BL_MODE) */
#define MIRISDR_MODE_AM                                 0x01
#define MIRISDR_MODE_VHF                                0x02
#define MIRISDR_MODE_B3                                 0x04
#define MIRISDR_MODE_B45                                0x08
#define MIRISDR_MODE_BL                                 0x10

/* reg0: 9 (AM_MODE2) */
#define MIRISDR_UPCONVERT_MIXER_OFF                     0
#define MIRISDR_UPCONVERT_MIXER_ON                      1

/* reg0: 10 (RF_SYNTH) */
#define MIRISDR_RF_SYNTHESIZER_OFF                      0
#define MIRISDR_RF_SYNTHESIZER_ON                       1

/* reg0: 11 (AM_PORT_SEL) */
#define MIRISDR_AM_PORT1                                0
#define MIRISDR_AM_PORT2                                1

/* reg0: 12:13 (FIL_MODE_SEL0, FIL_MODE_SEL1) */
#define MIRISDR_IF_MODE_2048KHZ                         0
#define MIRISDR_IF_MODE_1620KHZ                         1
#define MIRISDR_IF_MODE_450KHZ                          2
#define MIRISDR_IF_MODE_ZERO                            3

/* reg0: 14:16 (FIL_BW_SEL0 - FIL_BW_SEL2) */

/* reg0: 17:19 (XTAL_SEL0 - XTAL_SEL2) */

/* reg0: 20:22 (IF_LPMODE0 - IF_LPMODE2) */
#define MIRISDR_IF_LPMODE_NORMAL                        0
#define MIRISDR_IF_LPMODE_ONLY_Q                        1
#define MIRISDR_IF_LPMODE_ONLY_I                        2
#define MIRISDR_IF_LPMODE_LOW_POWER                     4

/* reg0: 23 (VCO_LPMODE) */
#define MIRISDR_VCO_LPMODE_NORMAL                       0
#define MIRISDR_VCO_LPMODE_LOW_POWER                    1

/*** Register 2: Synthesizer Programming ***/

/* reg2: 4:15 (FRAC0 - FRAC11) */

/* reg2: 16:21 (INT0 - INT5) */

/* reg2: 22 (LNACAL_EN) */
#define MIRISDR_LBAND_LNA_CALIBRATION_OFF               0
#define MIRISDR_LBAND_LNA_CALIBRATION_ON                1

/*** Register 6: RF Synthesizer Configuration ***/

/* reg5: 4:15 (THRESH0 - THRESH11) */

/* reg5: 16:21 (reserved) */
#define MIRISDR_RF_SYNTHESIZER_RESERVED_PROGRAMMING     0x28

typedef struct
{
    uint32_t low_cut;
    int mode;
    int upconvert_mixer_on;
    int am_port;
    int lo_div;
    uint32_t band_select_word;
} hw_switch_freq_plan_t;
