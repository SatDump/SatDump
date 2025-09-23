/*-----------------------------------------------------------------*/
/*!
  \file calcephstatetype.h
  \brief private API for calceph library
         state data type.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2011-2021, CNRS
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

#ifndef __CALCEPH_STATETYPE__
#define __CALCEPH_STATETYPE__

/*-----------------------------------------------------------------*/
/* private API */
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*! ephemeris state structure */
/*-----------------------------------------------------------------*/
typedef struct
{
    treal Position[3];          /*!<  position vector */
    treal Velocity[3];          /*!<  velocity vector */
    treal Acceleration[3];      /*!<  acceleration vector */
    treal Jerk[3];              /*!<  jerk vector (derivative of acceleration vector) */
    int order;                  /*!< =0 : if the Position vector is computed
                                   =1 : the Position+Velocity vector must be computed
                                   =2 : the Position+Velocity+Acceleration vector must be computed
                                   =3 : the Position+Velocity+Acceleration+Jerk vector must be computed
                                 */
} stateType;

/*! value of the default order (=1)*/
#define STATETYPE_ORDER_DEFAULTVALUE 1

/*! set the quantities PV to 0 depending on order */
void calceph_PV_set_0(double PV[ /*<=12 */ ], int order);

/*! set the quantities PV from stateType */
void calceph_PV_set_stateType(double PV[ /*<=12 */ ], const stateType state);

/*! set the quantities PV from -stateType */
void calceph_PV_neg_stateType(double PV[ /*<=12 */ ], const stateType state);

/*! set the quantities PV : PV-state */
void calceph_PV_sub_stateType(double PV[ /*<=12 */ ], const stateType state);

/*! set the quantities PV : PV+factor*state */
void calceph_PV_fma_stateType(double PV[ /*<=12 */ ], treal factor, const stateType state);

/*! set the quantities PV : PV-factor*state */
void calceph_PV_fms_stateType(double PV[ /*<=12 */ ], treal factor, const stateType state);

/*! set state to 0 */
void calceph_stateType_set_0(stateType * state, int order);

/*! state = state+a*b */
void calceph_stateType_fma(stateType * state, treal a, const stateType b);

/*! state = state/a */
void calceph_stateType_div_scale(stateType * state, treal a);

/*! state = state*a */
void calceph_stateType_mul_scale(stateType * state, treal a);

/*! multiply the 'time' quantities  by a**(derivative of time) */
void calceph_stateType_mul_time(stateType * state, treal a);

/*! divide the 'time' quantities  by a**(derivative of time) */
void calceph_stateType_div_time(stateType * state, treal a);

/*! perform the rotation of the state containing cartesian position and derivatives */
void calceph_stateType_rotate_PV(stateType * Planet, double rotationmatrix[3][3]);

/*! perform the rotation of the state containing euler angles */
int calceph_stateType_rotate_eulerangles(stateType * state, double rotationmatrix[3][3]);

/*! print debug information */
void calceph_stateType_debug(const stateType * state);
#endif
