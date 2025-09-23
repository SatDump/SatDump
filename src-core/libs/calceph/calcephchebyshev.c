/*-----------------------------------------------------------------*/
/*! 
 \file calcephchebyshev.c
 \brief private API for calceph library
 compute chebychev coefficients

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

 Copyright, 2011-2024, CNRS
   email of the author : Mickael.Gastineau@obspm.fr

*/
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* License  of this file :
 This file is "triple-licensed", you have to choose one  of the three licenses 
 below to apply on this file.
 
    CeCILL-C
    	The CeCILL-C license is close to the GNU LGPL.
    	( http://www.cecill.info/licences/Licence_CeCILL-C_V1-en.html )
   
 or CeCILL-B
        The CeCILL-B license is close to the BSD.
        (http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.txt)
  
 or CeCILL v2.1
      The CeCILL license is compatible with the GNU GPL.
      ( http://www.cecill.info/licences/Licence_CeCILL_V2.1-en.html )
 

This library is governed by the CeCILL-C, CeCILL-B or the CeCILL license under 
French law and abiding by the rules of distribution of free software.  
You can  use, modify and/ or redistribute the software under the terms 
of the CeCILL-C,CeCILL-B or CeCILL license as circulated by CEA, CNRS and INRIA  
at the following URL "http://www.cecill.info". 

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's author,  the holder of the
economic rights,  and the successive licensors  have only  limited
liability. 

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or 
data to be ensured and,  more generally, to use and operate it in the 
same conditions as regards security. 

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-C,CeCILL-B or CeCILL license and that you accept its terms.
*/
/*-----------------------------------------------------------------*/

#include "calcephconfig.h"
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#include "real.h"
#include "calcephchebyshev.h"

/*--------------------------------------------------------------------------*/
/*! compute the chebychev polynomials Cp[0:N-1] of order 0 using the time Tc
 @param Cp (out) array of N chebyshev polynomials of order 0 evaluated at the time Tc
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param Tc (in) time
 */
/*--------------------------------------------------------------------------*/
void calceph_chebyshev_order_0(double Cp[ /*N*/], int N, double Tc)
{
    int j;

    Cp[0] = REALC(1.0E0);
    Cp[1] = Tc;
    Cp[2] = REALC(2.0E0) * Tc * Tc - REALC(1.E0);

    for (j = 3; j < N; j++)
    {
        Cp[j] = REALC(2.0E0) * Tc * Cp[j - 1] - Cp[j - 2];
    }
}

/*--------------------------------------------------------------------------*/
/*! compute the chebychev polynomials Up[0:N-1] of order 1 evaluated at the time Tc 
 @param Up (out) array of N chebyshev polynomials of order 1 evaluated at the time Tc
 @param N (in) degree of the chebyshev Polynomial (size of Cp and Up)
 @param Tc (in) time
 @param Cp (in) array of N chebyshev polynomials of order 0 evaluated at the time Tc
 */
/*--------------------------------------------------------------------------*/
void calceph_chebyshev_order_1(double Up[ /*N*/], int N, double Tc, const double Cp[ /*N*/])
{
    int j;

    Up[0] = REALC(0.0E0);
    Up[1] = REALC(1.0E0);
    Up[2] = REALC(4.0E0) * Tc;
    for (j = 3; j < N; j++)
    {
        Up[j] = REALC(2.0E0) * Tc * Up[j - 1] + REALC(2.0E0) * Cp[j - 1] - Up[j - 2];
    }

}

/*--------------------------------------------------------------------------*/
/*! compute the chebychev polynomials Vp[0:N-1] of order 2 using the time Tc
 @param Vp (out) array of N chebyshev polynomials of order 2 evaluated at the time Tc
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param Tc (in) time
 @param Up (out) array of N chebyshev polynomials of order 1 evaluated at the time Tc
 */
/*--------------------------------------------------------------------------*/
void calceph_chebyshev_order_2(double Vp[ /*N*/], int N, double Tc, const double Up[ /*N*/])
{
    int j;

    Vp[0] = REALC(0.0E0);
    Vp[1] = REALC(0.E0);
    Vp[2] = REALC(4.E0);

    for (j = 3; j < N; j++)
    {
        Vp[j] = REALC(4.0E0) * Up[j - 1] + REALC(2.0E0) * Tc * Vp[j - 1] - Vp[j - 2];
    }
}

/*--------------------------------------------------------------------------*/
/*! compute the chebychev polynomials Wp[0:N-1] of order 2 using the time Tc
 @param Wp (out) array of N chebyshev polynomials of order 2 evaluated at the time Tc
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param Tc (in) time
 @param Vp (out) array of N chebyshev polynomials of order 1 evaluated at the time Tc
 */
/*--------------------------------------------------------------------------*/
void calceph_chebyshev_order_3(double Wp[ /*N*/], int N, double Tc, const double Vp[ /*N*/])
{
    int j;

    Wp[0] = REALC(0.0E0);
    Wp[1] = REALC(0.E0);
    Wp[2] = REALC(0.E0);

    for (j = 3; j < N; j++)
    {
        Wp[j] = REALC(6.0E0) * Vp[j - 1] + REALC(2.0E0) * Tc * Wp[j - 1] - Wp[j - 2];
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Cp[0:N-1]
  for Ncomp components. The stride between the data is 0
 @param Ncomp (in) number of components to evaluate
 @param A (in) array of N coefficients
 @param Cp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_0_stride_0(int Ncomp, double f[ /*3 */ ], int N, const double Cp[ /*N*/],
                                                    const double A[ /*N*/])
{
    int i, j;

    for (i = Ncomp; i < 3; i++)
        f[i] = 0.0E0;
    for (i = 0; i < Ncomp; i++)
    {
        double P_Sum = REALC(0.0E0);

        for (j = N - 1; j > -1; j--)
            P_Sum = P_Sum + A[j + i * N] * Cp[j];

        f[i] = P_Sum;
    }

}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Cp[0:N-1]
 for 3 components. The stride between the data is 3
 @param A (in) array of N coefficients
 @param Cp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_0_stride_3(double f[ /*3 */ ], int N, const double Cp[ /*N*/],
                                                    const double A[ /*N*/])
{
    int i, j;

    for (i = 0; i < 3; i++)
    {
        double P_Sum = REALC(0.0E0);

        for (j = N - 1; j > -1; j--)
            P_Sum = P_Sum + A[j + (3 + i) * N] * Cp[j];

        f[i] = P_Sum;
    }

}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Up[1:N-1]
 for Ncomp components. The stride between the data is 0
 @param Ncomp (in) number of components to evaluate
 @param A (in) array of N coefficients
 @param Up (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_1_stride_0(int Ncomp, double f[ /*3 */ ], int N, const double Up[ /*N*/],
                                                    const double A[ /*N*/], double factor)
{
    int i, j;

    for (i = Ncomp; i < 3; i++)
        f[i] = 0.0E0;
    for (i = 0; i < Ncomp; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 0; j--)
            V_Sum = V_Sum + A[j + i * N] * Up[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Up[1:N-1]
 for 3 components. The stride between the data is 3
 @param A (in) array of N coefficients
 @param Up (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_1_stride_3(double f[ /*3 */ ], int N, const double Up[ /*N*/],
                                                    const double A[ /*N*/], double factor)
{
    int i, j;

    for (i = 0; i < 3; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 0; j--)
            V_Sum = V_Sum + A[j + (3 + i) * N] * Up[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A[0:NA] and the chebychev polynomial using Up[1:N-1]
 for 3 components. The stride between the data is nstride
 @param A (in) array of N coefficients
 @param NA (in) number of elements per comonent of A
 @param Up (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Up)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 @param stride (in) stride between component
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_1_stride_n(double f[ /*3 */ ], int N, const double Up[ /*N*/],
                                                    const double A[ /*N*/], int NA, double factor, int Nstride)
{
    int i, j;

    for (i = 0; i < 3; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 0; j--)
            V_Sum = V_Sum + A[j + (Nstride + i) * NA] * Up[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Vp[2:N-1]
 for Ncomp components. The stride between the data is 0
 @param Ncomp (in) number of components to evaluate
 @param A (in) array of N coefficients
 @param Vp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_2_stride_0(int Ncomp, double f[ /*3 */ ], int N, const double Vp[ /*N*/],
                                                    const double A[ /*N*/], double factor)
{
    int i, j;

    for (i = Ncomp; i < 3; i++)
        f[i] = 0.0E0;
    for (i = 0; i < Ncomp; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 1; j--)
            V_Sum = V_Sum + A[j + i * N] * Vp[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Vp[2:N-1]
 for 3 components. The stride between the data is 3
 @param A (in) array of N coefficients
 @param Vp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_2_stride_3(double f[ /*3 */ ], int N, const double Vp[ /*N*/],
                                                    const double A[ /*N*/], double factor)
{
    int i, j;

    for (i = 0; i < 3; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 1; j--)
            V_Sum = V_Sum + A[j + (3 + i) * N] * Vp[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A[0:NA] and the chebychev polynomial using Vp[2:N-1]
 for 3 components. The stride between the data is nstride
 @param A (in) array of N coefficients
 @param NA (in) number of elmement per component of A
 @param Wp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 @param Nstride (in) stride between component
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_2_stride_n(double f[ /*3 */ ], int N, const double Vp[ /*N*/],
                                                    const double A[ /*N*/], int NA, double factor, int Nstride)
{
    int i, j;

    for (i = 0; i < 3; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 1; j--)
            V_Sum = V_Sum + A[j + (Nstride + i) * NA] * Vp[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Wp[3:N-1]
 for Ncomp components. The stride between the data is 0
 @param A (in) array of N coefficients
 @param Wp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 @param Ncomp (in) number of components to evaluate
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_3_stride_0(int Ncomp, double f[ /*3 */ ], int N, const double Wp[ /*N*/],
                                                    const double A[ /*N*/], double factor)
{
    int i, j;

    for (i = Ncomp; i < 3; i++)
        f[i] = 0.0E0;

    for (i = 0; i < Ncomp; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 2; j--)
            V_Sum = V_Sum + A[j + i * N] * Wp[j];

        f[i] = V_Sum * factor;
    }
}

/*--------------------------------------------------------------------------*/
/*! interpolate the function f using the coefficients A and the chebychev polynomial using Wp[1:N-1]
 for 3 components. The stride between the data is 3
 @param A (in) array of N coefficients
 @param Wp (in) array of N polynomials evaluated at the same point
 @param N (in) degree of the chebyshev Polynomial (size of Cp)
 @param f (out) array of 3 interpolated values
 @param factor (in) scale factor to apply to f
 */
/*--------------------------------------------------------------------------*/
void calceph_interpolate_chebyshev_order_3_stride_3(double f[ /*3 */ ], int N, const double Wp[ /*N*/],
                                                    const double A[ /*N*/], double factor)
{
    int i, j;

    for (i = 0; i < 3; i++)
    {

        double V_Sum = REALC(0.0E0);

        for (j = N - 1; j > 2; j--)
            V_Sum = V_Sum + A[j + (3 + i) * N] * Wp[j];

        f[i] = V_Sum * factor;
    }
}
