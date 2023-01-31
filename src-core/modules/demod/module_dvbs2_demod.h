#pragma once

#include "module_demod_base.h"
#include "common/dsp/filter/fir.h"
#include "common/dsp/clock_recovery/clock_recovery_mm.h"

#include "common/dsp/utils/freq_shift.h"

#include "dvbs2/dvbs2_pl_sync.h"
#include "dvbs2/dvbs2_pll.h"
#include "dvbs2/dvbs2_bb_to_soft.h"

#include "common/codings/dvb-s2/bbframe_descramble.h"
#include "common/codings/dvb-s2/bbframe_bch.h"
#include "common/codings/dvb-s2/bbframe_ldpc.h"

#include "common/dsp/demod/constellation.h"

#include "common/widgets/constellation_s2.h"
#include "common/widgets/value_plot.h"

namespace demod
{
    class DVBS2DemodModule : public BaseDemodModule
    {
    protected:
        std::shared_ptr<dsp::FIRBlock<complex_t>> rrc;
        std::shared_ptr<dsp::MMClockRecoveryBlock<complex_t>> rec;

        std::shared_ptr<dsp::FreqShiftBlock> freq_sh;

        std::shared_ptr<dvbs2::S2PLSyncBlock> pl_sync;
        std::shared_ptr<dvbs2::S2PLLBlock> s2_pll;
        std::shared_ptr<dvbs2::S2BBToSoft> s2_bb_to_soft;

        std::unique_ptr<dsp::RingBuffer<int8_t>> ring_buffer;
        std::unique_ptr<dsp::RingBuffer<uint8_t>> ring_buffer2;

        float d_rrc_alpha;
        int d_rrc_taps = 31;
        float d_loop_bw;
        float freq_propagation_factor = 0.01;

// This default will NOT work for 32-APSK, maybe we should tune per-requirements?
#define REC_ALPHA 1.7e-3
        float d_clock_gain_omega = pow(REC_ALPHA, 2) / 4.0;
        float d_clock_mu = 0.5f;
        float d_clock_gain_mu = REC_ALPHA;
        float d_clock_omega_relative_limit = 0.005f;

        int d_modcod;
        bool d_shortframes = false;
        bool d_pilots = false;
        float d_sof_thresold = 0.6;
        int d_max_ldpc_trials = 10;
        bool d_multithread_bch = false;

        // Running stuff
        bool should_stop = false;
        float current_freq = 0;

        // UI
        widgets::ConstellationViewerDVBS2 constellation_s2;
        int detected_modcod = -1;
        bool detected_shortframes = false;
        bool detected_pilots = false;
        float ldpc_trials;
        float bch_corrections;
        widgets::ValuePlotViewer ldpc_viewer;
        widgets::ValuePlotViewer bch_viewer;

        // Threads
        std::thread process_s2_th;
        std::thread process_bch_th;

        // DVB-S2 Stuff
        int frame_slot_count;
        dvbs2::dvbs2_constellation_t s2_constellation;
        dsp::constellation_type_t s2_constel_obj_type;
        dvbs2::dvbs2_framesize_t s2_framesize;
        dvbs2::dvbs2_code_rate_t s2_coderate;

        std::unique_ptr<dvbs2::BBFrameLDPC> ldpc_decoder;
        std::unique_ptr<dvbs2::BBFrameBCH> bch_decoder;
        std::unique_ptr<dvbs2::BBFrameDescrambler> descramber;

    public:
        DVBS2DemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
        ~DVBS2DemodModule();
        void init();
        void stop();
        void process();
        void process_s2();
        void process_s2_bch();
        void drawUI(bool window);

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}