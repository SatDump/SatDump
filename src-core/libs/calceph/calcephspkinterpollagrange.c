/*-----------------------------------------------------------------*/
/*! 
  \file calcephspkinterpollagrange.c
  
  performthe Langrange interpolation

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2019, CNRS
   email of the author : Mickael.Gastineau@obspm.fr

  History:
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
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#if HAVE_MATH_H
#include <math.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephspice.h"
#include "util.h"
#include "calcephinternal.h"

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_Lagrange_an(int m, const double *y, const double *x, double *a);

/*--------------------------------------------------------------------------*/
/*!  This function performs the Lagrange Interpolation.
 
 @param m (in) degree of the polynomial.
 @param y (in) array of m+1 points
 @param x (in) array of m+1 times
 @param a (out) array of the coefficients of the polynomial of degree m
 */
/*--------------------------------------------------------------------------*/
static void calceph_spk_interpol_Lagrange_an(int m, const double *x, const double *y, double *a)
{
    int i, k;

    double fact;

    double travail[1000];

    travail[0] = y[0];
    for (i = 1; i <= m; i++)
    {
        travail[i] = y[i];
    }
    /* compute the coefficients */
    a[0] = travail[0];
    fact = 1.E0;
    for (k = 1; k <= m; k++)
    {
        fact = fact * k;
        for (i = 0; i <= m - k; i++)
        {
            travail[i] = (travail[i] - travail[i + 1]) / (x[i] - x[i + k]);
        }
        a[k] = travail[0];
    }
}

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors
     using the Lagrange Interpolation.                                      
                                          
 @param S (in) number of valid points in drecord .
 It assumes that S <1000/2 .                                                                        
 @param drecord (in) array of 6*S points: (x,y,z, dx/dt, dy/dt, dz/dt), (x,y,z , ...), (x, y, z,....)              
 @param depoch (in) array of S times              
 @param TimeJD0  (in) Time in seconds from J2000 for which position is desired (Julian Date)
 @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
 @param Planet (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units                                 
   Planet has velocity in ephemerides units                                 
*/
/*--------------------------------------------------------------------------*/
void calceph_spk_interpol_Lagrange(int S, const double *drecord, const double *depoch,
                                   double TimeJD0, double Timediff, stateType * Planet)
{
    int i, j, k, q, r, s;

    double a[1000], x[1000], y[1000], vy[1000], b[1000];

    for (i = 0; i < 3; i++)
    {
        double P_Sum, V_Sum;

        for (k = 0; k < S; k++)
        {
            x[k] = depoch[k];
            y[k] = drecord[6 * k + i];  /* position */
            vy[k] = drecord[6 * k + i + 3]; /* derivative */
        }
        /* compute the coefficients of the Lagrange interpolation polynomial  */
        calceph_spk_interpol_Lagrange_an(S - 1, x, y, a);

        /* evaluate the polynomial at time tp  : position */
        P_Sum = a[S - 1];
        for (k = S - 2; k >= 0; k--)
        {
            double delta = ((TimeJD0 - x[k]) + Timediff);

            P_Sum = P_Sum * delta + a[k];
        }
        Planet->Position[i] = P_Sum;

        /* evaluate the derivative of the polynomial at time tp  : velocity */
        if (Planet->order >= 1)
        {
            /* compute the coefficients of the Lagrange interpolation polynomial  */
            calceph_spk_interpol_Lagrange_an(S - 1, x, vy, b);
            V_Sum = b[S - 1];
            for (k = S - 2; k >= 0; k--)
            {
                double delta = ((TimeJD0 - x[k]) + Timediff);

                V_Sum = V_Sum * delta + b[k];
            }
            Planet->Velocity[i] = V_Sum;
        }

        /* evaluate the second derivative of the polynomial at time tp  : acceleration */
        if (Planet->order >= 2)
        {
            double A_Sum = 0.E0;

            for (k = 0; k <= S - 1; k++)
            {
                double sumdelta = 0.E0;

                for (q = 0; q < k; q++)
                {
                    for (r = 0; r < k; r++)
                    {
                        double proddelta = 1.E0;

                        if (q != r)
                        {
                            for (j = 0; j < k; j++)
                            {
                                if (j != r && j != q)
                                    proddelta *= ((TimeJD0 - x[j]) + Timediff);
                            }
                            sumdelta += proddelta;
                        }
                    }
                }
                A_Sum = A_Sum + a[k] * sumdelta;
            }
            Planet->Acceleration[i] = A_Sum;
        }

        /* evaluate the second derivative of the polynomial at time tp  : jerks */
        if (Planet->order >= 3)
        {
            double J_Sum = 0.E0;

            for (k = 0; k <= S - 1; k++)
            {
                double sumdelta = 0.E0;

                for (q = 0; q < k; q++)
                {
                    for (r = 0; r < k; r++)
                    {
                        if (q != r)
                        {
                            for (s = 0; s < k; s++)
                            {
                                double proddelta = 1.E0;

                                if (s != r && s != q)
                                {
                                    for (j = 0; j < k; j++)
                                    {
                                        if (j != r && j != q && j != s)
                                            proddelta *= ((TimeJD0 - x[j]) + Timediff);
                                    }
                                    sumdelta += proddelta;
                                }
                            }
                        }
                    }
                }
                J_Sum = J_Sum + a[k] * sumdelta;
            }
            Planet->Jerk[i] = J_Sum;
        }
    }
}
