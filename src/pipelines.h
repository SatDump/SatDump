#include "pipeline.h"

Pipeline metop_ahrpt = {"metop_ahrpt", {{"soft", {{"qpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                  {"symbolrate", "2333333"},
                                                                  {"agc_rate", "0.00001"},
                                                                  {"rrc_alpha", "0.5"},
                                                                  {"rrc_taps", "31"},
                                                                  {"costas_bw", "0.002"},
                                                                  {"buffer_size", "8192"}}}}},
                                        {"cadu", {{"metop_ahrpt_decoder", {{"viterbi_outsync_after", "5"}, {"viterbi_ber_thresold", "0.170"}, {"soft_symbols", "1"}}}}},
                                        {"products", {{"metop_avhrr", {}}, {"metop_mhs", {}}, {"metop_amsu", {}}, {"metop_iasi", {{"write_all", "0"}}}, {"metop_gome", {{"write_all", "0"}}}}}}};

Pipeline fengyun3ab_ahrpt = {"fengyun3_ab_ahrpt", {{"soft", {{"qpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                             {"symbolrate", "2800000"},
                                                                             {"agc_rate", "0.00001"},
                                                                             {"rrc_alpha", "0.5"},
                                                                             {"rrc_taps", "31"},
                                                                             {"costas_bw", "0.002"},
                                                                             {"buffer_size", "8192"}}}}},
                                                   {"cadu", {{"fengyun_ahrpt_decoder", {{"viterbi_outsync_after", "5"}, {"viterbi_ber_thresold", "0.170"}, {"soft_symbols", "1"}, {"invert_second_viterbi", "0"}}}}},
                                                   {"products", {{"fengyun_virr", {}}}}}};

Pipeline fengyun3c_ahrpt = {"fengyun3_c_ahrpt", {{"soft", {{"qpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                           {"symbolrate", "2600000"},
                                                                           {"agc_rate", "0.00001"},
                                                                           {"rrc_alpha", "0.5"},
                                                                           {"rrc_taps", "31"},
                                                                           {"costas_bw", "0.002"},
                                                                           {"buffer_size", "8192"}}}}},
                                                 {"cadu", {{"fengyun_ahrpt_decoder", {{"viterbi_outsync_after", "5"}, {"viterbi_ber_thresold", "0.170"}, {"soft_symbols", "1"}, {"invert_second_viterbi", "0"}}}}},
                                                 {"products", {{"fengyun_virr", {}}}}}};

Pipeline fengyun3abc_mpt = {"fengyun3_abc_mpt", {{"soft", {{"qpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                           {"symbolrate", "18700000"},
                                                                           {"agc_rate", "0.00001"},
                                                                           {"rrc_alpha", "0.35"},
                                                                           {"rrc_taps", "31"},
                                                                           {"costas_bw", "0.0063"},
                                                                           {"buffer_size", "8192"}}}}},
                                                 {"cadu", {{"fengyun_mpt_decoder", {{"viterbi_outsync_after", "5"}, {"viterbi_ber_thresold", "0.150"}, {"soft_symbols", "1"}, {"invert_second_viterbi", "1"}}}}},
                                                 {"products", {{"fengyun_mersi1", {}}}}}};

Pipeline fengyun3d_ahrpt = {"fengyun3_d_ahrpt", {{"soft", {{"qpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                           {"symbolrate", "30000000"},
                                                                           {"agc_rate", "0.00001"},
                                                                           {"rrc_alpha", "0.35"},
                                                                           {"rrc_taps", "31"},
                                                                           {"costas_bw", "0.0063"},
                                                                           {"buffer_size", "8192"}}}}},
                                                 {"cadu", {{"fengyun_ahrpt_decoder", {{"viterbi_outsync_after", "5"}, {"viterbi_ber_thresold", "0.170"}, {"soft_symbols", "1"}, {"invert_second_viterbi", "1"}}}}},
                                                 {"products", {{"fengyun_mersi2", {}}}}}};

Pipeline aqua_db = {"aqua_db", {{"soft", {{"oqpsk_demod", {/*{"samplerate", "6000000"},*/
                                                           {"symbolrate", "7500000"},
                                                           {"agc_rate", "0.00001"},
                                                           {"rrc_alpha", "0.5"},
                                                           {"rrc_taps", "31"},
                                                           {"costas_bw", "0.006"},
                                                           {"buffer_size", "8192"}}}}},
                                {"cadu", {{"aqua_db_decoder", {}}}},
                                {"products", {{"eos_modis", {{"terra_mode", "0"}}}, {"aqua_airs", {}}, {"aqua_amsu", {}}}}}};

Pipeline noaa_hrpt = {"noaa_hrpt", {{"frames", {{"noaa_hrpt_demod", {/*{"samplerate", "6000000"},*/
                                                                     {"buffer_size", "8192"}}}}},
                                    {"products", {
                                                     {"noaa_avhrr", {}} /*,
                                                        {"aqua_airs", {}},
                                                        {"aqua_amsu", {}}*/
                                                 }}}};

Pipeline meteor_hrpt = {"meteor_hrpt", {{"dem", {{"meteor_hrpt_demod", {/*{"samplerate", "6000000"},*/
                                                                        {"buffer_size", "8192"}}}}},
                                        {"cadu", {{"meteor_hrpt_decoder", {}}}},
                                        {"products", {
                                                         {"meteor_msumr", {}} /*,
                                                         {"aqua_airs", {}},
                                                         {"aqua_amsu", {}}*/
                                                     }}}};

Pipeline npp_hrd = {"npp_hrd", {{"soft", {{"qpsk_demod", {/*{"samplerate", "6000000"},*/
                                                          {"symbolrate", "15000000"},
                                                          {"agc_rate", "0.0001f"},
                                                          {"rrc_alpha", "0.7"},
                                                          {"rrc_taps", "31"},
                                                          {"costas_bw", "0.001"},
                                                          {"buffer_size", "8192"}}}}},
                                {"cadu", {{"npp_hrd_decoder", {{"viterbi_outsync_after", "5"}, {"viterbi_ber_thresold", "0.170"}, {"soft_symbols", "1"}}}}},
                                {"products", {{"jpss_viirs", {{"npp_mode", "1"}}}, {"jpss_atms", {{"npp_mode", "1"}}
                                                                                       /*,
                                                                     {"metop_mhs", {}},
                                                                     {"metop_amsu", {}},
                                                                     {"metop_iasi", {{"write_all", "0"}}},
                                                                     {"metop_gome", {{"write_all", "0"}}}*/
                                                                                   }}}}};

Pipeline proba1_dump = {"proba1_dump", {
                                           {"soft", {{"bpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                     {"symbolrate", "2000000"},
                                                                     {"agc_rate", "0.000001"},
                                                                     {"rrc_alpha", "0.5"},
                                                                     {"rrc_taps", "31"},
                                                                     {"costas_bw", "0.004"},
                                                                     {"buffer_size", "8192"}}}}},
                                           {"cadu", {{"proba_s_decoder", {{"derandomize", "0"}}}}} /*,
                            {"products", {{"fengyun_mersi2", {}}}}*/
                                       }};

Pipeline proba2_dump = {"proba2_dump", {
                                           {"soft", {{"bpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                     {"symbolrate", "2000000"},
                                                                     {"agc_rate", "0.000001"},
                                                                     {"rrc_alpha", "0.5"},
                                                                     {"rrc_taps", "31"},
                                                                     {"costas_bw", "0.004"},
                                                                     {"buffer_size", "8192"}}}}},
                                           {"cadu", {{"proba_s_decoder", {{"derandomize", "0"}}}}} /*,
                            {"products", {{"fengyun_mersi2", {}}}}*/
                                       }};

Pipeline probavs_s_dump = {"probav_s_dump", {
                                                {"soft", {{"bpsk_demod", {/*{"samplerate", "6000000"},*/
                                                                          {"symbolrate", "1919000"},
                                                                          {"agc_rate", "0.000001"},
                                                                          {"rrc_alpha", "0.5"},
                                                                          {"rrc_taps", "31"},
                                                                          {"costas_bw", "0.004"},
                                                                          {"buffer_size", "8192"}}}}},
                                                {"cadu", {{"proba_s_decoder", {{"derandomize", "1"}}}}} /*,
                            {"products", {{"fengyun_mersi2", {}}}}*/
                                            }};