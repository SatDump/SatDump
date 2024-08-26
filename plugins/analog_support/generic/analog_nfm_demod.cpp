#include "analog_nfm_demod.h"
#include "common/dsp/block.h"
#include "common/dsp/buffer.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/utils/agc.h"
#include <complex.h>
#include <memory>


namespace generic_analog
{
	NFMDemod::NFMDemod(std::shared_ptr<dsp::stream<complex_t>> input, double samplerate, double final_samplerate)
		: HierBlock(input)
	{
		// AGC
		agc = std::make_shared<dsp::AGCBlock<complex_t>>(input, 1e-2, 1.0f, 1.0f, 65536);
		
		// Resampler to BW
		res = std::make_shared<dsp::RationalResamplerBlock<complex_t>>(agc->output_stream, samplerate, final_samplerate);

		// Quadrature Demod
		qua = std::make_shared<dsp::QuadratureDemodBlock>(res->output_stream, dsp::hz_to_rad(final_samplerate / 2, final_samplerate));
	}

	void NFMDemod::start()
	{
		agc->start();
		res->start();
		qua->start();
	}

	void NFMDemod::stop()
	{
		agc->stop();
		res->stop();
		qua->stop();
		qua->output_stream->stopReader();
	}
}
