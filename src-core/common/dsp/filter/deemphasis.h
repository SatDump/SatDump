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
		float lastSamp;
		float lastSampImag;
		float lastSampReal;
		double tau;
		void work();

		int in_buffer = 0;
		int d_nsamples = 0;

		float alpha;
		double quad_sampl_rate = 0;

	public:
		DeEmphasisBlock(std::shared_ptr<dsp::stream<T>> input, double quad_sampl_rate, double tau);
		~DeEmphasisBlock();
	};
}
