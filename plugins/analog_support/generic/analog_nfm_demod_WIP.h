#pragma once

#include "common/dsp/block.h"
#include "common/dsp/demod/quadrature_demod.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/utils/agc.h"
#include "module_generic_analog_demod.h"
#include "modules/demod/module_demod_base.h"
#include <complex.h>
#include <memory>

namespace generic_analog
{
	class NFMDemod : public dsp::HierBlock<complex_t, float>
	{
		private:
			std::shared_ptr<dsp::AGCBlock<complex_t>> agc;
			std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
			std::shared_ptr<dsp::QuadratureDemodBlock> qua;
		public:
			NFMDemod(std::shared_ptr<dsp::stream<complex_t>> input, double samplerate, double final_samplerate);
			void start();
			void stop();

			//float getFreq()
			//{
			//	if (pll)
			//		return pll->getFreq;
			//	else
			//		return 0;
			//}
	};
}
