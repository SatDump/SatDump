#pragma once

#include "module.h"
#include <complex>
#include <thread>
#include <fstream>
#include "deframer.h"
#include "common/dsp/lib/random.h"
#include "common/dsp/agc.h"
#include "common/dsp/fir.h"
#include "common/dsp/moving_average.h"
#include "common/dsp/clock_recovery_mm.h"
#include "common/dsp/file_source.h"
#include "common/dsp/bpsk_carrier_pll.h"
#include "common/widgets/constellation.h"
#include "common/snr_estimator.h"
#include "common/widgets/snr_plot.h"

namespace meteor
{
    class METEORHRPTDemodModule : public ProcessingModule
    {
    protected:
        std::shared_ptr<dsp::FileSourceBlock> file_source;
        std::shared_ptr<dsp::AGCBlock> agc;
        std::shared_ptr<dsp::CCFIRBlock> rrc;
        std::shared_ptr<dsp::BPSKCarrierPLLBlock> pll;
        std::shared_ptr<dsp::FFMovingAverageBlock> mov;
        std::shared_ptr<dsp::FFMMClockRecoveryBlock> rec;

        const int d_samplerate;
        const int d_buffer_size;

        uint8_t *bits_buffer;

        std::ofstream data_out;

        uint8_t byteToWrite;
        int inByteToWrite = 0;

        std::vector<uint8_t> getBytes(uint8_t *bits, int length);

        std::atomic<size_t> filesize;
        std::atomic<size_t> progress;

        M2M4SNREstimator snr_estimator;
        float snr, peak_snr;

        // UI Stuff
        widgets::ConstellationViewer constellation;
        widgets::SNRPlotViewer snr_plot;

    public:
        METEORHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
        ~METEORHRPTDemodModule();
        void init();
        void stop();
        void process();
        void drawUI(bool window);
        std::vector<ModuleDataType> getInputTypes();
        std::vector<ModuleDataType> getOutputTypes();

    public:
        static std::string getID();
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters);
    };
} // namespace meteor
