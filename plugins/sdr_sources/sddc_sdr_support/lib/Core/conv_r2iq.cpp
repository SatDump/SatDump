#include "license.txt"  
/*
The ADC input real stream of 16 bit samples (at Fs = 64 Msps in the example) is converted to:
- 32 Msps float Fs/2 complex stream, or
- 16 Msps float Fs/2 complex stream, or
-  8 Msps float Fs/2 complex stream, or
-  4 Msps float Fs/2 complex stream, or
-  2 Msps float Fs/2 complex stream.
The decimation factor is selectable from HDSDR GUI sampling rate selector

The name r2iq as Real 2 I+Q stream

*/

#include "conv_r2iq.h"
#include "config.h"
#include "RadioHandler.h"

#include <assert.h>
#include <utility>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>



conv_r2iq::conv_r2iq() :
	r2iqControlClass()
{
    fprintf(stderr,"conv_r2iq created\n");
}

conv_r2iq::~conv_r2iq()
{
    if(IsOn())
        TurnOff();
    fprintf(stderr,"conv_r2iq deleted\n");
}


float conv_r2iq::setFreqOffset(float offset)
{
	return 0.0;
}

void conv_r2iq::TurnOn()
{
    r2iqControlClass::TurnOn();
	this->bufIdx = 0;

	fprintf(stderr,"conv_r2iq::TurnOn()\n");
	r2iq_thread = std::thread(
			[this] ()
				{ return this->r2iqThreadf(); });
}

void conv_r2iq::TurnOff(void) {
    r2iqControlClass::TurnOff();

	inputbuffer->Stop();
	outputbuffer->Stop();
	r2iq_thread.join();
}

void conv_r2iq::Init(float gain, ringbuffer<int16_t> *input, ringbuffer<float>* obuffers)
{
	this->inputbuffer = input;    // set to the global exported by main_loop
	this->outputbuffer = obuffers;  // set to the global exported by main_loop

	(void) gain;

    DbgPrintf((char *) "r2iqCntrl initialization\n");
}

#ifdef _WIN32
	//  Windows, assumed MSVC
	#include <intrin.h>
	#define cpuid(info, x)    __cpuidex(info, x, 0)
	#define DETECT_AVX
#elif defined(__x86_64__)
	//  GCC Intrinsics, x86 only
	#include <cpuid.h>
	#define cpuid(info, x)  __cpuid_count(x, 0, info[0], info[1], info[2], info[3])
	#define DETECT_AVX
#elif defined(__arm__)
	#define DETECT_NEON
	#include <sys/auxv.h>
	#include <asm/hwcap.h>
	static bool detect_neon()
	{
		unsigned long caps = getauxval(AT_HWCAP);
		return (caps & HWCAP_NEON);
	}
#else
#error Compiler does not identify an x86 or ARM core..
#endif

void * conv_r2iq::r2iqThreadf()
{
	DbgPrintf("Hardware Capability: all SIMD features (AVX, AVX2, AVX512) deactivated\n");
	return r2iqThreadf_def();
}

void * conv_r2iq::r2iqThreadf_def()
{

	while (IsOn()) {
		const int16_t *dataADC;  // pointer to input data
		float *pout;

		{
			std::unique_lock<std::mutex> lk(mutexR2iqControl);
			dataADC = inputbuffer->getReadPtr();
    		pout = outputbuffer->getWritePtr();

			if (!IsOn())
				return 0;
		}


		// @todo: move the following int16_t conversion to (32-bit) float
		// directly inside the following loop (for "k < fftPerBuf")
		//   just before the forward fft "fftwf_execute_dft_r2c" is called
		// idea: this should improve cache/memory locality
		if (!this->getRand())        // plain samples no ADC rand set
			convert_float<false>(dataADC, &pout[bufIdx], transferSamples);
		else
			convert_float<true>(dataADC, &pout[bufIdx], transferSamples);

		inputbuffer->ReadDone();
		bufIdx += transferSamples * 2;
		// decimate in frequency plus tuning
		if(bufIdx >= outputbuffer->getBlockSize())
		{
//    		printf("ibs=%d, obs=%d\n",inputbuffer->getBlockSize(),outputbuffer->getBlockSize());
			outputbuffer->WriteDone();
			bufIdx = 0;
		}
	} // while(run)
//    DbgPrintf((char *) "r2iqThreadf idx %d pthread_exit %u\n",(int)th->t, pthread_self());
	return 0;
}
