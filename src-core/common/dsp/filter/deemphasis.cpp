

#include "common/dsp/filter/deemphasis.h"
#include "common/dsp/buffer.h"
#include <memory>
#include <type_traits>

namespace dsp
{
	template<typename T>
	DeEmphasisBlock<T>::DeEmphasisBlock(std::shared_ptr<dsp::stream<T>> input, double samplerate, double tau, int ntaps) : Block<T, T>(input)
	{
	}
	

	template <typename T>
    	DeEmphasisBlock<T>::~DeEmphasisBlock()
    	{
    	    volk_free(buffer);
    	}


	template <typename T>
	void DeEmphasisBlock<T>::work()
	{
		int nsamples = Block<T, T>::input_stream->read();
		if (nsamples <= 0)
		{
        	    	Block<T, T>::input_stream->flush();
        		return;
        	}

        	memcpy(&buffer[ntaps], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
        	Block<T, T>::input_stream->flush();

		if constexpr (std::is_same_v<T, float>)
		{
			for (int i = 0; i < nsamples; i++)
			{


			}
		}
		if constexpr (std::is_same_v<T, complex_t>)
		{
			for (int i = 0; i < nsamples; i++)
			{


			}
		}

		//Block<T, T>::input_stream->flush();
		Block<T, T>::output_stream->swap(nsamples);

		memmove(&buffer[0], &buffer[nsamples], ntaps * sizeof(T));
	}



    template class DeEmphasisBlock<complex_t>;
    template class DeEmphasisBlock<float>;
}

/*
		ntaps |= 1;

		if (tau > 1.0e-9)
		{
        		// copied from fm_emph.py in gr-analog
        		double  w_c;    // Digital corner frequency
        		double  w_ca;   // Prewarped analog corner frequency
        		double  k, z1, p1, b0;
        		double  fs = (double)samplerate;

        		w_c = 1.0 / tau;
        		w_ca = 2.0 * fs * tan(w_c / (2.0 * fs));

        		// Resulting digital pole, zero, and gain term from the bilinear
        		// transformation of H(s) = w_ca / (s + w_ca) to
        		// H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
        		k = -w_ca / (2.0 * fs);
        		z1 = -1.0;
        		p1 = (1.0 + k) / (1.0 - k);
        		b0 = -k / (1.0 - k);

        		d_fftaps[0] = b0;
        		d_fftaps[1] = -z1 * b0;
        		d_fbtaps[0] = 1.0;
        		d_fbtaps[1] = -p1;
		}
		else
		{
			d_fftaps[0] = 1.0;
        		d_fftaps[1] = 0.0;
        		d_fbtaps[0] = 0.0;
        		d_fbtaps[1] = 0.0;
		}
	}

}
*/
