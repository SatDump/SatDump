#include "analog_ssb_demod.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/utils/freq_shift.h"
#include <memory>

namespace generic_analog
{
	SSBDemod::SSBDemod(std::shared_ptr<dsp::stream<complex_t>> input, double samplerate, double final_samplerate, bool ssb_mode)
		: HierBlock(input)
	{
		// AGC
		agc = std::make_shared<dsp::AGCBlock<complex_t>>(input, 1e-2, 1.0f, 1.0f, 65536);
		
		// Resample to BW
		res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, samplerate, final_samplerate);

		// Frequency Shift Block
		if (ssb_mode)
			fsb = std::make_shared<dsp::FreqShiftBlock>(res->output_stream, final_samplerate, -final_samplerate / 2);
		
		fsb = std::make_shared<dsp::FreqShiftBlock>(res->output_stream, final_samplerate, final_samplerate / 2);

	}

	void SSBDemod::start()
	{
		agc->start();
		res->start();
		fsb->start();
	}

	void SSBDemod::stop()
	{
		agc->stop();
		res->stop();
		fsb->stop();
		fsb->output_stream->stopReader();
		fsb->output_stream->stopWriter();
	}
}

