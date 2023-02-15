#pragma once

#include "common/dsp/complex.h"

/* From GNU Radio, adapted */

//! \brief SNR Estimator using 2nd and 4th-order moments.
/*! \ingroup snr_blk
 *
 *  An SNR estimator for M-PSK signals that uses 2nd (M2) and 4th
 *  (M4) order moments. This estimator uses knowledge of the
 *  kurtosis of the signal (\f$k_a)\f$ and noise (\f$k_w\f$) to make its
 *  estimation. We use Beaulieu's approximations here to M-PSK
 *  signals and AWGN channels such that \f$k_a=1\f$ and \f$k_w=2\f$. These
 *  approximations significantly reduce the complexity of the
 *  calculations (and computations) required.
 *
 *  Reference:
 *  D. R. Pauluzzi and N. C. Beaulieu, "A comparison of SNR
 *  estimation techniques for the AWGN channel," IEEE
 *  Trans. Communications, Vol. 48, No. 10, pp. 1681-1691, 2000.
 */
class M2M4SNREstimator
{
private:
    float d_y1, d_y2;
    float d_alpha, d_beta;
    float d_signal, d_noise;

public:
    /*! Constructor
     *
     *  Parameters:
     *  \param alpha: the update rate of internal running average
     *  calculations.
     */
    M2M4SNREstimator(float alpha = 0.001);
    ~M2M4SNREstimator() {}

    void update(complex_t *input, int size);
    float snr();
    float signal();
    float noise();
};