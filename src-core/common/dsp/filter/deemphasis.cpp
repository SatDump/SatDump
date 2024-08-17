

#include "common/dsp/filter/deemphasis.h"
#include "common/dsp/buffer.h"
#include <memory>
#include <type_traits>

namespace dsp
{
	template<typename T>
	DeEmphasisBlock<T>::DeEmphasisBlock(std::shared_ptr<dsp::stream<T>> input, double quad_sampl_rate, double tau) : Block<T, T>(input)
	{
		float dt = 1.0f / quad_sampl_rate;
		alpha = dt / (tau + dt);
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

		for (int i = 0; i < nsamples; i++)
		{
			T input = Block<T, T>::input_stream->readBuf[i];
			T output = Block<T, T>::output_stream->readBuf[i];

			//float dt = 1.0f / quad_sampl_rate;
			//alpha = dt / (tau + dt);

			if constexpr (std::is_same_v<T, float>)
			{
				output[0] = (alpha * input[0]) + ((1 - alpha) * lastSamp);
				for (int j = 1; j < nsamples; j++)
				{
					output[j] = (alpha * input[j]) + ((1 - alpha) * output[j -1]);
				}
				lastSamp = output[nsamples - 1];
			}

			if constexpr (std::is_same_v<T, complex_t>)
			{
				output[0].imag = (alpha * input[0].imag) + ((1 - alpha) * lastSampImag);
				output[0].real = (alpha * input[0].real) + ((1 - alpha) * lastSampReal);
				for (int j = 1; j < nsamples; j++)
				{
					output[j].imag = (alpha * input[j].imag) + ((1 - alpha) * output[j - 1].imag);
					output[j].real = (alpha * input[j].real) + ((1 - alpha) * output[j - 1].real);
				}

				lastSampImag = output[nsamples - 1].imag;
				lastSampReal = output[nsamples - 1].real;

			}

			Block<T, T>::output_stream->writeBuf[i] = output;
		}

		Block<T, T>::input_stream->flush();
		Block<T, T>::output_stream->swap(nsamples);
	}
}






		/*
        	memcpy(&buffer[ntaps], Block<T, T>::input_stream->readBuf, nsamples * sizeof(T));
        	Block<T, T>::input_stream->flush();




		/*
		if constexpr (std::is_same_v<T, float>)
		{
			for (int i = 0; i < nsamples; i++)
			{


        			// copied from fm_emph.py in gr-analog
        			double  w_c;    // Digital corner frequency
        			double  w_ca;   // Prewarped analog corner frequency
        			double  k, z1, p1, b0;
        			double  fs = (double)quad_sampl_rate;

        			w_c = 1.0 / tau;
        			w_ca = 2.0 * fs * tan(w_c / (2.0 * fs));


        			// Resulting digital pole, zero, and gain term from the bilinear
        			// transformation of H(s) = w_ca / (s + w_ca) to
        			// H(z) = b0 (1 - z1 z^-1)/(1 - p1 z^-1)
        			k = -w_ca / (2.0 * fs);
        			z1 = -1.0;
        			p1 = (1.0 + k) / (1.0 - k);
        			b0 = -k / (1.0 - k);
				
				d_fftaps[0] = 1.0;
        			d_fftaps[1] = 0.0;
        			d_fbtaps[0] = 0.0;
        			d_fbtaps[1] = 0.0;

			}
		}
		if constexpr (std::is_same_v<T, complex_t>)
		{
			for (int i = 0; i < nsamples; i++)
			{
        			// copied from fm_emph.py in gr-analog
        			double  w_c;    // Digital corner frequency
        			double  w_ca;   // Prewarped analog corner frequency
        			double  k, z1, p1, b0;
        			double  fs = (double)quad_sampl_rate;

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
