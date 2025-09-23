/*-----------------------------------------------------------------*/
/*!
  \file calcephtxtpckorient.c
  \brief perform the computation of the orientation (euler angles) on text PCK KERNEL ephemeris data file
         using the constants BODY..._POLE_RA, BODY..._POLE_DE, BODY..._POLE_PM,
         from a text PCK KERNEL Ephemeris data.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2019-2024, CNRS
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
/* enable M_PI with windows sdk */
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifndef __cplusplus
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#endif                          /* __cplusplus */
#include <ctype.h>

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephspice.h"
#include "util.h"
#include "calcephinternal.h"

/*--------------------------------------------------------------------------*/
/*! Return the orientation of the object
    for a given target at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param spicetarget (in) "target" object (NAIF ID number)
   @param unit (in) sum of CALCEPH_UNIT_???
   @param order (in) order of the computation
   =0 : Positions are  computed
   =1 : Position+Velocity  are computed
   =2 : Position+Velocity+Acceleration  are computed
   =3 : Position+Velocity+Acceleration+Jerk  are computed
   @param PV (out) quantities of the "target" object
   PV[0:3*(order+1)-1] are valid
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_orient_unit(const struct calcephbin_spice *eph, double JD0, double time,
                               int spicetarget, int unit, int order, double PV[ /*<=12 */ ])
{
    stateType statetarget;

    int res;
    int found_pole_ra, found_pole_de, found_pole_pm;

    int found_nut_prec_ra, found_nut_prec_de, found_nut_prec_pm, found_nut_prec_angles;

    char name_pole_ra[50], name_pole_de[50], name_pole_pm[50];

    char name_nut_prec_ra[50], name_nut_prec_de[50], name_nut_prec_pm[50], name_nut_prec_angles[50];

    double vd_pole_ra[100], vd_pole_de[100], vd_pole_pm[100];

    double vd_nut_prec_ra[100], vd_nut_prec_de[100], vd_nut_prec_pm[100], vd_nut_prec_angles[100];

    int bodybary = spicetarget / 100;

    int j, p, k;
    int ephunit;

    /* locate the required constants BODY..._POLE_RA, BODY..._POLE_DE, BODY..._POLE_PM */
    calceph_snprintf(name_pole_ra, 50, "BODY%d_POLE_RA", spicetarget);
    calceph_snprintf(name_pole_de, 50, "BODY%d_POLE_DEC", spicetarget);
    calceph_snprintf(name_pole_pm, 50, "BODY%d_PM", spicetarget);
    found_pole_ra = calceph_spice_getconstant_vd(eph, name_pole_ra, vd_pole_ra, 100);
    found_pole_de = calceph_spice_getconstant_vd(eph, name_pole_de, vd_pole_de, 100);
    found_pole_pm = calceph_spice_getconstant_vd(eph, name_pole_pm, vd_pole_pm, 100);
    if (found_pole_ra == 0 || found_pole_de == 0 || found_pole_pm == 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Orientation for the target object %d is not available in the ephemeris file.\n", spicetarget);
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    /* locate the optional constants BODY..._NUT_PREC_RA, BODY..._NUT_PREC_DE, BODY..._NUT_PREC_PM */
    calceph_snprintf(name_nut_prec_ra, 50, "BODY%d_NUT_PREC_RA", spicetarget);
    calceph_snprintf(name_nut_prec_de, 50, "BODY%d_NUT_PREC_DEC", spicetarget);
    calceph_snprintf(name_nut_prec_pm, 50, "BODY%d_NUT_PREC_PM", spicetarget);
    calceph_snprintf(name_nut_prec_angles, 50, "BODY%d_NUT_PREC_ANGLES", spicetarget);
    found_nut_prec_ra = calceph_spice_getconstant_vd(eph, name_nut_prec_ra, vd_nut_prec_ra, 100);
    found_nut_prec_de = calceph_spice_getconstant_vd(eph, name_nut_prec_de, vd_nut_prec_de, 100);
    found_nut_prec_pm = calceph_spice_getconstant_vd(eph, name_nut_prec_pm, vd_nut_prec_pm, 100);
    found_nut_prec_angles = calceph_spice_getconstant_vd(eph, name_nut_prec_angles, vd_nut_prec_angles, 100);
    if (found_nut_prec_angles == 0 && bodybary * 100 + 1 <= spicetarget && spicetarget <= bodybary * 100 + 99 &&
        0 < bodybary && bodybary < 10)
    {
        /* try with the barycenter of the system for the satellites */
        calceph_snprintf(name_nut_prec_angles, 50, "BODY%d_NUT_PREC_ANGLES", bodybary);
        found_nut_prec_angles = calceph_spice_getconstant_vd(eph, name_nut_prec_angles, vd_nut_prec_angles, 100);
    }

    if (found_nut_prec_ra > 0)
    {
        if (found_nut_prec_angles / 2 < found_nut_prec_ra || found_nut_prec_angles / 2 < found_nut_prec_de ||
            found_nut_prec_angles / 2 < found_nut_prec_pm)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Wrong size for the array %s.\n", name_nut_prec_angles);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
    }

    /* check the order */
    if (order >= 2)
    {
        /* GCOVR_EXCL_START */
        fatalerror("order>=2 is not supported for the orientation using a text PCK files");
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    /*  pole RA/Dec terms in the PCK are in degrees and degrees/century; the rates here have been converted
       to rad/day. Prime meridian terms in the PCK are in degrees and degrees/day; the rate here has been
       converted to rad/day.  */

    /* evaluations  */
    for (p = 0; p <= order; p++)
    {
        const double DeltaDay = ((JD0 - 2.451545E+06) + time);
        const double DeltaCentury = DeltaDay / 36525E0;
        double ra = 0., de = 0., pm = 0.;

        switch (p)
        {

            case 0:
                for (j = found_pole_ra - 1; j >= p; j--)
                    ra = ra * DeltaCentury + vd_pole_ra[j];
                for (j = found_pole_de - 1; j >= p; j--)
                    de = de * DeltaCentury + vd_pole_de[j];
                for (j = found_pole_pm - 1; j >= p; j--)
                    pm = pm * DeltaDay + vd_pole_pm[j];
                for (j = 0; j < found_nut_prec_ra; j++)
                {
                    double v =
                        (vd_nut_prec_angles[2 * j] + vd_nut_prec_angles[2 * j + 1] * DeltaCentury) * M_PI / 180.E0;
                    ra += vd_nut_prec_ra[j] * sin(v);
                }
                for (j = 0; j < found_nut_prec_de; j++)
                {
                    double v =
                        (vd_nut_prec_angles[2 * j] + vd_nut_prec_angles[2 * j + 1] * DeltaCentury) * M_PI / 180.E0;
                    de += vd_nut_prec_de[j] * cos(v);
                }
                for (j = 0; j < found_nut_prec_pm; j++)
                {
                    double v =
                        (vd_nut_prec_angles[2 * j] + vd_nut_prec_angles[2 * j + 1] * DeltaCentury) * M_PI / 180.E0;
                    pm += vd_nut_prec_pm[j] * sin(v);
                }
                break;

            case 1:
                for (j = found_pole_ra - 1; j >= p; j--)
                {
                    double x = 1.;
                    int k;

                    for (k = 1; k < j; k++)
                        x *= DeltaCentury;
                    ra += j * vd_pole_ra[j] * x / 36525E0;

                }
                for (j = found_pole_de - 1; j >= p; j--)
                {
                    double x = 1.;
                    int k;

                    for (k = 1; k < j; k++)
                        x *= DeltaCentury;
                    de += j * vd_pole_de[j] * x / 36525E0;
                }

                for (j = found_pole_pm - 1; j >= p; j--)
                {
                    double x = 1.;
                    int k;

                    for (k = 1; k < j; k++)
                        x *= DeltaDay;
                    pm += j * vd_pole_pm[j] * x;

                }

                for (j = 0; j < found_nut_prec_ra; j++)
                {
                    double v =
                        (vd_nut_prec_angles[2 * j] + vd_nut_prec_angles[2 * j + 1] * DeltaCentury) * M_PI / 180.E0;
                    ra += vd_nut_prec_angles[2 * j + 1] * M_PI / 180.E0 / 36525E0 * vd_nut_prec_ra[j] * cos(v);
                }
                for (j = 0; j < found_nut_prec_de; j++)
                {
                    double v =
                        (vd_nut_prec_angles[2 * j] + vd_nut_prec_angles[2 * j + 1] * DeltaCentury) * M_PI / 180.E0;
                    de -= vd_nut_prec_angles[2 * j + 1] * M_PI / 180.E0 / 36525E0 * vd_nut_prec_de[j] * sin(v);
                }
                for (j = 0; j < found_nut_prec_pm; j++)
                {
                    double v =
                        (vd_nut_prec_angles[2 * j] + vd_nut_prec_angles[2 * j + 1] * DeltaCentury) * M_PI / 180.E0;
                    pm += vd_nut_prec_angles[2 * j + 1] * M_PI / 180.E0 / 36525E0 * vd_nut_prec_pm[j] * cos(v);
                }
                break;

            default:
                /* GCOVR_EXCL_START */
                fatalerror("internal error : unknown order : %d\n", order);
                /* GCOVR_EXCL_STOP */
                break;
        }

        /*    angle  * ASCALE = ( RA   + pi/2 )
           1

           angle  * ASCALE = ( pi/2 - DEC )
           2

           angle  * ASCALE = ( W )
           3  
         */

        PV[3 * p + 0] = ra;
        PV[3 * p + 1] = -de;
        PV[3 * p + 2] = pm;
    }
    PV[0] = 90.E0 + PV[0];
    PV[1] = 90.E0 + PV[1];

    statetarget.order = order;
    for (k = 0; k <= 2; k++)
    {
        statetarget.Position[k] = fmod(PV[3 * 0 + k], 360.E0);
        if (order >= 1)
            statetarget.Velocity[k] = PV[3 * 1 + k];
        if (order >= 2)
            statetarget.Acceleration[k] = PV[3 * 2 + k];
        if (order >= 3)
            statetarget.Jerk[k] = PV[3 * 3 + k];
    }
    /* change to rad */
    calceph_stateType_mul_scale(&statetarget, M_PI / 180.E0);

    ephunit = CALCEPH_UNIT_DAY + CALCEPH_UNIT_RAD;
    res = calceph_spice_unit_convert_orient(&statetarget, ephunit, unit);
    calceph_PV_set_stateType(PV, statetarget);

    return res;
}
