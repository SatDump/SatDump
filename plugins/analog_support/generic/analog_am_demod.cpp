#include "analog_am_demod.h"
#include "common/dsp/buffer.h"
#include "common/dsp/pll/pll_carrier_tracking.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/utils/complex_to_mag.h"
#include <complex.h>
#include <memory>


namespace generic_analog
{
	AMDemod::AMDemod(std::shared_ptr<dsp::stream<complex_t>> input, double samplerate, double final_samplerate, float d_pll_bw, float d_pll_max_offset)
		: HierBlock(input)
	{
		// AGC
		agc = std::make_shared<dsp::AGCBlock<complex_t>>(input, 1e-2, 1.0f, 1.0f, 65536);
		
		// Resampler to BW
		res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, samplerate, final_samplerate);

		// Pll
		pll = std::make_shared<dsp::PLLCarrierTrackingBlock>(res->output_stream, d_pll_bw, d_pll_max_offset, -d_pll_max_offset);

		// Complex to Mag
		ctm = std::make_shared<dsp::ComplexToMagBlock>(pll->output_stream);
	}

	void AMDemod::start()
	{
		agc->start();
		res->start();
		pll->start();
		ctm->start();
	}

	void AMDemod::stop()
	{
		agc->stop();
		res->stop();
		pll->stop();
		ctm->stop();
		ctm->output_stream->stopReader();
	}
}
