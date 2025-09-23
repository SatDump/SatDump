/*-----------------------------------------------------------------*/
/*!
  \file calcephchebyshev.h
  \brief private API for calceph library
         compute chebychev coefficients

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2011-2024, CNRS
   email of the author : Mickael.Gastineau@obspm.fr

 History :
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

#ifndef __CALCEPH_CHEBYSHEV__
#define __CALCEPH_CHEBYSHEV__

/*-----------------------------------------------------------------*/
/* private API */
/*-----------------------------------------------------------------*/

/*! compute the chebychev coefficients Cp[0:N-1] of order 0 using the time Tc  */
void calceph_chebyshev_order_0(double Cp[ /*N*/], int N, double Tc);

/*! compute the chebychev coefficients Up[0:N-1] of order 1 using the time Tc and  chebychev coefficients
 * of order 0 */
void calceph_chebyshev_order_1(double Up[ /*N*/], int N, double Tc, const double Cp[ /*N*/]);

/*! compute the chebychev coefficients Vp[0:N-1] of order 2 using the time Tc  */
void calceph_chebyshev_order_2(double Vp[ /*N*/], int N, double Tc, const double Up[ /*N*/]);

/*! compute the chebychev coefficients Wp[0:N-1] of order 3 using the time Tc and  chebychev coefficients
 * of order 0 */
void calceph_chebyshev_order_3(double Wp[ /*N*/], int N, double Tc, const double Vp[ /*N*/]);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Cp[0:N-1]
 for Ncomp components. The stride between the data is 0 */
void calceph_interpolate_chebyshev_order_0_stride_0(int Ncomp, double f[ /*3 */ ], int N,
                                                    const double Cp[ /*N*/], const double A[ /*N*/]);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Cp[0:N-1]
 for 3 components. The stride between the data is 3  */
void calceph_interpolate_chebyshev_order_0_stride_3(double f[ /*3 */ ], int N, const double Cp[ /*N*/],
                                                    const double A[ /*N*/]);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Up[1:N-1]
 for Ncomp components. The stride between the data is 0 */
void calceph_interpolate_chebyshev_order_1_stride_0(int Ncomp, double f[ /*3 */ ], int N,
                                                    const double Up[ /*N*/], const double A[ /*N*/], double factor);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Up[1:N-1]
 for 3 components. The stride between the data is 3  */
void calceph_interpolate_chebyshev_order_1_stride_3(double f[ /*3 */ ], int N, const double Up[ /*N*/],
                                                    const double A[ /*N*/], double factor);

/*! interpolate the function f using the coefficients A[0:NA] and the chebychev polynomial using
 Up[1:N-1] for 3 components. The stride between the data is nstride  */
void calceph_interpolate_chebyshev_order_1_stride_n(double f[ /*3 */ ], int N, const double Up[ /*N*/],
                                                    const double A[ /*N*/], int NA, double factor, int Nstride);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Vp[2:N-1]
 for Ncomp components. The stride between the data is 0 */
void calceph_interpolate_chebyshev_order_2_stride_0(int Ncomp, double f[ /*3 */ ], int N,
                                                    const double Vp[ /*N*/], const double A[ /*N*/], double factor);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Vp[2:N-1]
 for 3 components. The stride between the data is 3  */
void calceph_interpolate_chebyshev_order_2_stride_3(double f[ /*3 */ ], int N, const double Vp[ /*N*/],
                                                    const double A[ /*N*/], double factor);

/*! interpolate the function f using the coefficients A[0:NA] and the chebychev polynomial using
 Vp[2:N-1] for 3 components. The stride between the data is nstride  */
void calceph_interpolate_chebyshev_order_2_stride_n(double f[ /*3 */ ], int N, const double Vp[ /*N*/],
                                                    const double A[ /*N*/], int NA, double factor, int Nstride);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Wp[2:N-1]
 for Ncomp components. The stride between the data is 0 */
void calceph_interpolate_chebyshev_order_3_stride_0(int Ncomp, double f[ /*3 */ ], int N,
                                                    const double Wp[ /*N*/], const double A[ /*N*/], double factor);

/*! interpolate the function f using the coefficients A and the chebychev polynomial using Wp[2:N-1]
 for 3 components. The stride between the data is 3  */
void calceph_interpolate_chebyshev_order_3_stride_3(double f[ /*3 */ ], int N, const double Wp[ /*N*/],
                                                    const double A[ /*N*/], double factor);

#endif
