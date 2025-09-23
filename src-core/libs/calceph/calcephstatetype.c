/*-----------------------------------------------------------------*/
/*! 
  \file calcephstatetype.c
  \brief perform operations on the data strcture statetype.

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2016-2021, CNRS
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
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_MATH_H
#include <math.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"
#include "calcephstatetype.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/*! set the quantities PV to 0 depending on order
 @param order (in) order of the computation
  =0 : Positions are  computed
  =1 : Position+Velocity  are computed
  =2 : Position+Velocity+Acceleration  are computed
  =3 : Position+Velocity+Acceleration+Jerk  are computed
@param PV (out) quantities of the "target" object
 PV[0:3*(order+1)-1] are valid
 */
/*--------------------------------------------------------------------------*/
void calceph_PV_set_0(double PV[ /*<=12 */ ], int order)
{
    int j;

    int n = (order + 1) * 3;

    for (j = 0; j < n; j++)
    {
        PV[j] = 0.;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities PV from stateType 
 @param PV (out) copied values from statetype.
  the number of valid values depends on statetype
 @param state (in) input values
 */
/*--------------------------------------------------------------------------*/
void calceph_PV_set_stateType(double PV[ /*<=12 */ ], const stateType state)
{
    int j;

    switch (state.order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                PV[j + 9] = state.Jerk[j];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                PV[j + 6] = state.Acceleration[j];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                PV[j + 3] = state.Velocity[j];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                PV[j] = state.Position[j];
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities PV from -stateType  : negate
 @param PV (out) copied values from statetype.
 the number of valid values depends on statetype
 @param state (in) input values
 */
/*--------------------------------------------------------------------------*/
void calceph_PV_neg_stateType(double PV[ /*<=12 */ ], const stateType state)
{
    int j;

    switch (state.order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                PV[j + 9] = -state.Jerk[j];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                PV[j + 6] = -state.Acceleration[j];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                PV[j + 3] = -state.Velocity[j];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                PV[j] = -state.Position[j];
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities PV : PV-=state
 @param PV (inout) PV=PV-state
 the number of valid values depends on statetype
 @param state (in) input values
 */
/*--------------------------------------------------------------------------*/
void calceph_PV_sub_stateType(double PV[ /*<=12 */ ], const stateType state)
{
    int j;

    switch (state.order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                PV[j + 9] -= state.Jerk[j];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                PV[j + 6] -= state.Acceleration[j];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                PV[j + 3] -= state.Velocity[j];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                PV[j] -= state.Position[j];
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities PV : PV+factor*state
 @param PV (inout) PV+factor*state.
 the number of valid values depends on statetype
 @param factor (in)  input values
 @param state (in) input values
 */
/*--------------------------------------------------------------------------*/
/*!  */
void calceph_PV_fma_stateType(double PV[ /*<=12 */ ], treal factor, const stateType state)
{
    int j;

    switch (state.order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                PV[j + 9] += factor * state.Jerk[j];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                PV[j + 6] += factor * state.Acceleration[j];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                PV[j + 3] += factor * state.Velocity[j];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                PV[j] += factor * state.Position[j];
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities PV : PV-factor*state
 @param PV (inout) PV-factor*state.
 the number of valid values depends on statetype
 @param factor (in)  input values
 @param state (in) input values
 */
/*--------------------------------------------------------------------------*/
void calceph_PV_fms_stateType(double PV[ /*<=12 */ ], treal factor, const stateType state)
{
    int j;

    switch (state.order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                PV[j + 9] -= factor * state.Jerk[j];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                PV[j + 6] -= factor * state.Acceleration[j];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                PV[j + 3] -= factor * state.Velocity[j];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                PV[j] -= factor * state.Position[j];
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set state to 0 depending on order
 @param state (out) all fields are set to 0 and order is set using the order input.
 @param order (in) order of the computation
 =0 : Positions are  computed
 =1 : Position+Velocity  are computed
 =2 : Position+Velocity+Acceleration  are computed
 =3 : Position+Velocity+Acceleration+Jerk  are computed
 =-1 : default behavior : Position and velocity for position quantites and time for time scales
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_set_0(stateType * state, int order)
{
    int j;

    state->order = order;
    for (j = 0; j < 3; j++)
    {
        state->Position[j] = 0.E0;
        state->Velocity[j] = 0.E0;
        state->Acceleration[j] = 0.E0;
        state->Jerk[j] = 0.E0;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities state : state+a*b
 @param state (inout) state+a*b
 @param a (in)  input values
 @param b (in) input values
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_fma(stateType * state, treal a, const stateType b)
{
    int j;

    switch (state->order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                state->Jerk[j] += a * b.Jerk[j];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                state->Acceleration[j] += a * b.Acceleration[j];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                state->Velocity[j] += a * b.Velocity[j];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                state->Position[j] += a * b.Position[j];
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities state : state/a
 @param state (inout) state/a
 @param a (in)  input values
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_div_scale(stateType * state, treal a)
{
    int j;

    switch (state->order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                state->Jerk[j] /= a;
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                state->Acceleration[j] /= a;
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                state->Velocity[j] /= a;
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                state->Position[j] /= a;
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! set the quantities state : state*a
 @param state (inout) state*a
 @param a (in)  input values
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_mul_scale(stateType * state, treal a)
{
    int j;

    switch (state->order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                state->Jerk[j] *= a;
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                state->Acceleration[j] *= a;
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                state->Velocity[j] *= a;
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                state->Position[j] *= a;
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/* multiply the 'time' quantities by a**(derivative of time)
 @param state (inout) state*a**(derivative of time)
 @param a (in)  scaling factor
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_mul_time(stateType * state, treal a)
{
    int j;

    switch (state->order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                state->Jerk[j] *= (a * a * a);
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                state->Acceleration[j] *= a * a;
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                state->Velocity[j] *= a;
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/* divide the 'time' quantities by a**(derivative of time)
 @param state (inout) state/a**(derivative of time)
 @param a (in)  scaling factor
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_div_time(stateType * state, treal a)
{
    int j;

    switch (state->order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                state->Jerk[j] /= (a * a * a);
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                state->Acceleration[j] /= a * a;
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                state->Velocity[j] /= a;
            }
            break;
        default:
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*! perform the rotation of all the quantities according to the rotation matrix
 @param rotationmatrix (in) rotation matrix 3x3
 @param Planet (inout) position and velocity of the planet
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_rotate_PV(stateType * Planet, double rotationmatrix[3][3])
{
    stateType newstate;

    int j;

    newstate.order = Planet->order;

    switch (Planet->order)
    {
        case 3:
            for (j = 0; j < 3; j++)
            {
                newstate.Jerk[j] =
                    rotationmatrix[j][0] * Planet->Jerk[0] + rotationmatrix[j][1] * Planet->Jerk[1] +
                    rotationmatrix[j][2] * Planet->Jerk[2];
            }                   /* no break *//* FALLTHRU */
        case 2:
            for (j = 0; j < 3; j++)
            {
                newstate.Acceleration[j] =
                    rotationmatrix[j][0] * Planet->Acceleration[0] + rotationmatrix[j][1] * Planet->Acceleration[1] +
                    rotationmatrix[j][2] * Planet->Acceleration[2];
            }                   /* no break *//* FALLTHRU */
        case 1:
            for (j = 0; j < 3; j++)
            {
                newstate.Velocity[j] =
                    rotationmatrix[j][0] * Planet->Velocity[0] + rotationmatrix[j][1] * Planet->Velocity[1] +
                    rotationmatrix[j][2] * Planet->Velocity[2];
            }                   /* no break *//* FALLTHRU */
        case 0:
            for (j = 0; j < 3; j++)
            {
                newstate.Position[j] =
                    rotationmatrix[j][0] * Planet->Position[0] + rotationmatrix[j][1] * Planet->Position[1] +
                    rotationmatrix[j][2] * Planet->Position[2];
            }
            break;
        default:
            break;
    }

    *Planet = newstate;
}

/*--------------------------------------------------------------------------*/
/*! perform the rotation of all the quantities according to the rotation matrix.
 it updates the euler angles
 @details 
  return 0 on error.
  return 1 on success.

 @param rotationmatrix (in) rotation matrix 3x3
 @param state (inout) euler angles of the orientation of the planet
*/
/*--------------------------------------------------------------------------*/

int calceph_stateType_rotate_eulerangles(stateType * state, double rotationmatrix[3][3])
{
    int res;
    union
    {
        double mat[3][3];
        double vec[9];
    } tkframe_matrix;

    double matrixframe1[3][3];

    int tkframe_axes[3] = { 3, 1, 3 };

    double angles[] = { -state->Position[0], -state->Position[1], -state->Position[2] };
    int j;
    int k;
    double rotationmatrixtr[3][3];

    calceph_txtfk_creatematrix_eulerangles(tkframe_matrix.vec, angles, tkframe_axes);
    calceph_matrix3x3_prod(matrixframe1, tkframe_matrix.mat, rotationmatrix);

    for (j = 0; j < 3; j++)
    {
        for (k = 0; k < 3; k++)
        {
            rotationmatrixtr[j][k] = matrixframe1[k][j];
        }
    }
    res = calceph_txtfk_createeulerangles_matrix(angles, rotationmatrixtr);
    state->Position[0] = angles[2];
    state->Position[1] = angles[1];
    state->Position[2] = angles[0];

    if (res != 0 && state->order > 0)
    {
        fatalerror("Derivatives (or higher) are not computed with a non ICRF frame\n");
        res = 0;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! print the debug information for statetype
 @param state (in) display values
 */
/*--------------------------------------------------------------------------*/
void calceph_stateType_debug(const stateType * state)
{
    int j;

    printf("order = %d\n", state->order);
    for (j = 0; j < 3; j++)
        printf("%23.16E %23.16E %23.16E %23.16E\n", state->Position[j], state->Velocity[j], state->Acceleration[j],
               state->Jerk[j]);
    printf("distance to center = %23.16E\n",
           sqrt(state->Position[0] * state->Position[0] + state->Position[1] * state->Position[1] +
                state->Position[2] * state->Position[2]));
}
