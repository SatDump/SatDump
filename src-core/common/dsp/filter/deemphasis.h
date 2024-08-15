#pragma once

#include "common/dsp/block.h"
#include <memory>
#include <vector>

namespace dsp
{
	template <typename T>
	class DeEmphasisBlock : public Block<T, T>
	{
	private:
		T *buffer;
		float **taps;
		float *ataps;
		float *btaps;
		int ntaps;
		double tau;
		void work();

		int in_buffer = 0;
		int d_nsamples = 0;

		float alpha = 1.0f;
		double quad_sampl_rate = 0;

    		/* De-emph IIR filter taps */
    		std::vector<double> d_fftaps;  /*! Feed forward taps. */
    		std::vector<double> d_fbtaps;  /*! Feed back taps. */

	public:
		DeEmphasisBlock(std::shared_ptr<dsp::stream<T>> input, double quad_sampl_rate, double tau, int ntaps);
		~DeEmphasisBlock();
	};
}
