#pragma once

#include "common/dsp/block.h"
#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp/utils/agc.h"
#include "common/dsp/utils/freq_shift.h"
#include <complex.h>
#include <memory>

namespace generic_analog
{
	class SSBDemod : public dsp::HierBlock<complex_t, complex_t>
	{
		private:
			std::shared_ptr<dsp::AGCBlock<complex_t>> agc;
			std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> res;
			std::shared_ptr<dsp::FreqShiftBlock> fsb;

		private:
			void work()
			{
				int dat_size = input_stream->read();

				if (dat_size <= 0)
				{
					input_stream->flush();
					return;
				}


				for (int i = 0; i < dat_size; i++)
					input_stream->readBuf[i] = complex_t(input_stream->readBuf[i].real, input_stream->readBuf[i].imag);


				input_stream->flush();
				output_stream->swap(dat_size);

			}

		public:
			SSBDemod(std::shared_ptr<dsp::stream<complex_t>> input, double samplerate, double final_samplerate, bool ssb_mode = false);
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
