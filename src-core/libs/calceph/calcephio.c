/*-----------------------------------------------------------------*/
/*!
  \file calcephio.c
  \brief perform I/O on ephemeris data file
         interpolate INPOP and JPL Ephemeris data.
         compute position and velocity from binary ephemeris file.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, LTE, CNRS, Observatoire de Paris.

   Copyright, 2008-2025, CNRS
   email of the author : Mickael.Gastineau@obspm.fr

  History:
 \bug M. Gastineau 01/03/05 : thread-safe version
 \bug M. Gastineau 09/09/05 : support of the libration
 \bug M. Gastineau 18/11/05 : incrase the number of coefficients
 \bug M. Gastineau 22/03/06 : support of non-native "endian" files
 \bug M. Gastineau 11/05/06 : support of DE411 and DE414
 \bug M. Gastineau 13/11/06 : support of km or UA units for  INPOP
 \bug M. Gastineau 22/07/08 : support of TT-TDB for INPOP
 \bug M. Gastineau 17/10/08 : support of DE421
 \bug M. Gastineau 07/10/09 : fix i/o error detection and handle large files >2GB
    on 32-bits operating systems
 \note M. Gastineau 04/05/10 : support DE423
 \note M. Gastineau 20/05/11 : remove explicit depedencies to record size for JPL DExxx
 \note M. Gastineau 21/05/11 : support asteroid in INPOP file format 2.0
 \note M. Gastineau 10/06/11 : support SPICE kernel file
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
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/* declarations for old compatibility layer */
void calcephinit(const char *filename);

void calceph(treal JD0, treal time, int target, int center, treal PV[6]);

/*--------------------------------------------------------------------------*/
/*!
     Open the "filename" ephemeris file .
     select correct algorithm against file type.

  @return descriptor of the ephemeris

  @param fileName (in) A character string giving the name of an ephemeris data file.
*/
/*--------------------------------------------------------------------------*/
t_calcephbin *calceph_open(const char *fileName)
{
    const char *fileName_array[1] = { fileName };
    return calceph_open_array(1, fileName_array);
}

/*--------------------------------------------------------------------------*/
/*!
     Open n "filename" ephemeris files .

  @return descriptor of the ephemeris

  @param n (in) number of elemnts of the array fileName.
  @param fileName_array (in) array of character strings giving the name of the n ephemeris data files.
*/
/*--------------------------------------------------------------------------*/
t_calcephbin *calceph_open_array(int n, const char *const fileName_array[ /*n */ ])
{
    return calceph_open_array2(n, (char **) &fileName_array[0]);
}

/*--------------------------------------------------------------------------*/
/*!
     Open n "filename" ephemeris files .

  @return descriptor of the ephemeris

  @param n (in) number of elemnts of the array fileName.
  @param fileName_array (in) array of character strings giving the name of the n ephemeris data files.
*/
/*--------------------------------------------------------------------------*/
t_calcephbin *calceph_open_array2(int n, char **fileName_array)
{
    t_calcephbin *res;

    int k;

    buffer_error_t buffer_error;

    if (n <= 0)
        return NULL;

    /* allocate memory */
    res = (t_calcephbin *) malloc(sizeof(t_calcephbin));
    if (res == NULL)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't allocate memory for t_calcephbin\nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));

        return NULL;
        /* GCOVR_EXCL_STOP */
    }
    res->etype = CALCEPH_unknown;
    res->data.spkernel.list = NULL;

    for (k = 0; k < n; k++)
    {
        int originalde = 1;

        struct SPICEkernel *spicekernel;

        t_HeaderBlock H1;

        FILE *file;

        const char *fileName = fileName_array[k];

#if DEBUG
        printf("fileName=%p %s\n", fileName_array[k], fileName);
#endif

    /*--------------------------------------------------------------------------*/
        /*  Open ephemeris file.                                                    */
    /*--------------------------------------------------------------------------*/
        file = fopen(fileName, "rb");
        if (file == NULL)
        {
            fatalerror("Unable to open ephemeris file: '%s'\nSystem error : '%s'\n", fileName,
                       calceph_strerror_errno(buffer_error));
            calceph_close(res);
            return NULL;
        }

    /*--------------------------------------------------------------------------*/
        /*  Read the header block                                                   */
    /*--------------------------------------------------------------------------*/
        if (fread(&H1, sizeof(t_HeaderBlock), 1, file) != 1)
        {
            originalde = 0;
            fseek(file, 0L, SEEK_SET);
            if (fread(&H1, 7 * sizeof(char), 1, file) != 1)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't read header block from the ephemeris file '%s'\nSystem error : '%s'\n",
                           fileName, calceph_strerror_errno(buffer_error));
                fclose(file);
                calceph_close(res);
                return NULL;
                /* GCOVR_EXCL_STOP */
            }
        }

    /*--------------------------------------------------------------------------*/
        /* select algorithm */
    /*--------------------------------------------------------------------------*/
        if (strncmp(H1.label[0], "NAIF/DA", 7) == 0)
        {                       /* SPK files */
            fatalerror("Unsupported old SPICE kernel (NAIF/DAF).\n");
            fclose(file);
            calceph_close(res);
            return NULL;
        }
        else if (strncmp(H1.label[0], "DAF/SPK", 7) == 0)
        {                       /* SPK files */
            if (res->etype == CALCEPH_ebinary)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't open SPICE kernel and INPOP/original DE files at the same time.\n");
                fclose(file);
                calceph_close(res);
                return NULL;
                /* GCOVR_EXCL_STOP */
            }
            res->etype = CALCEPH_espice;
            if ((spicekernel = calceph_spice_addfile(&res->data.spkernel)) == NULL)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_spk_open(file, fileName, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_spice_tablelinkbody_addfile(&res->data.spkernel, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
        }
        else if (strncmp(H1.label[0], "DAF/PCK", 7) == 0)
        {                       /* binary PCK files */
            if (res->etype == CALCEPH_ebinary)
            {
                fatalerror("Can't open SPICE kernel and INPOP/original DE files at the same time.\n");
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            res->etype = CALCEPH_espice;
            if ((spicekernel = calceph_spice_addfile(&res->data.spkernel)) == NULL)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_binpck_open(file, fileName, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_spice_tablelinkbody_addfile(&res->data.spkernel, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
        }
        else if (strncmp(H1.label[0], "KPL/PCK", 7) == 0)
        {                       /* text PCK files */
            if (res->etype == CALCEPH_ebinary)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't open SPICE kernel and INPOP/original DE files at the same time.\n");
                fclose(file);
                calceph_close(res);
                return NULL;
                /* GCOVR_EXCL_STOP */
            }
            res->etype = CALCEPH_espice;
            if ((spicekernel = calceph_spice_addfile(&res->data.spkernel)) == NULL)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_txtpck_open(file, fileName, res->data.spkernel.clocale, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_txtpck_merge_incrementalassignment(res->data.spkernel.list, spicekernel) == 0
                || calceph_spice_tablelinkbody_addfile(&res->data.spkernel, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
        }
        else if (strncmp(H1.label[0], "KPL/FK", 6) == 0)
        {                       /* text frame files */
            if (res->etype == CALCEPH_ebinary)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't open SPICE kernel and INPOP/original DE files at the same time.\n");
                fclose(file);
                calceph_close(res);
                return NULL;
                /* GCOVR_EXCL_STOP */
            }
            res->etype = CALCEPH_espice;
            if ((spicekernel = calceph_spice_addfile(&res->data.spkernel)) == NULL)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_txtfk_open(file, fileName, res->data.spkernel.clocale, spicekernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
        }
        else if (strncmp(H1.label[0], "KPL/MK", 6) == 0)
        {                       /* text frame files */
            if (res->etype == CALCEPH_ebinary)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't open SPICE kernel and INPOP/original DE files at the same time.\n");
                fclose(file);
                calceph_close(res);
                return NULL;
                /* GCOVR_EXCL_STOP */
            }
            res->etype = CALCEPH_espice;
            if (calceph_txtmk_open(file, fileName, &res->data.spkernel) == 0)
            {
                fclose(file);
                calceph_close(res);
                return NULL;
            }
        }
        else if (originalde == 1)
        {                       /* original INPOP/DE files */
            if (k >= 1)
            {
                fatalerror("Can't open multiple INPOP/original DE files at the same time.\n");
                fclose(file);
                calceph_close(res);
                return NULL;
            }
            if (calceph_inpop_open(file, fileName, &res->data.binary) == 0)
            {
                calceph_close(res);
                return NULL;
            }
            res->etype = CALCEPH_ebinary;
        }
        else
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't read header block from the ephemeris file '%s'\nSystem error : '%s'\n", fileName,
                       calceph_strerror_errno(buffer_error));
            fclose(file);
            calceph_close(res);
            return NULL;
            /* GCOVR_EXCL_STOP */
        }
    }

    /* create the links initial links for the spice kernels */
    if (res->etype == CALCEPH_espice)
    {
        if (calceph_spice_tablelinkbody_createinitiallink(&res->data.spkernel) == 0)
        {
            calceph_close(res);
            return NULL;
        }
    }

    return res;
}

/*--------------------------------------------------------------------------*/
/*! duplicate the handle of the ephemeris data file
  return null if an error occurs.
  return a new new handle
 @param eph (inout) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
#if 0
t_calcephbin *calceph_fdopen(t_calcephbin * eph)
{
    t_calcephbin *res = NULL;

    res = (t_calcephbin *) malloc(sizeof(t_calcephbin));
    if (res == NULL)
    {
        fatalerror("Can't allocate memory for t_calcephbin\nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));

        return NULL;
    }
    res->etype = eph->etype;

    switch (eph->etype)
    {
            /*case CALCEPH_espice:
               res->data.spkernel = calceph_spice_fdopen(&eph->data.spkernel);
               break;

               case CALCEPH_ebinary:
               res->data.binary = calceph_inpop_fdopen(&eph->data.binary);
               break;

               case CALCEPH_unknown:
               break; */

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_fdopen\n");
            res = NULL;
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}
#endif

/*--------------------------------------------------------------------------*/
/*! Close the  ephemeris file and destroy the ephemeris descriptor
 @param eph (inout) ephemeris descriptor to be destroyed
*/
/*--------------------------------------------------------------------------*/
void calceph_close(t_calcephbin * eph)
{
    switch (eph->etype)
    {
        case CALCEPH_espice:
            calceph_spice_close(&eph->data.spkernel);
            break;

        case CALCEPH_ebinary:
            calceph_inpop_close(&eph->data.binary);
            break;

            /* GCOVR_EXCL_START */
        case CALCEPH_unknown:
            break;

        default:
            fatalerror("Unknown ephemeris type in calceph_close\n");
            break;
            /* GCOVR_EXCL_STOP */
    }
    free(eph);
}

/*--------------------------------------------------------------------------*/
/*! Open the specified ephemeris file
 @param filename (in) nom du fichier
*/
/*--------------------------------------------------------------------------*/
void calcephinit(const char *filename)
{
    calceph_sopen(filename);
}

/*--------------------------------------------------------------------------*/
/*! Return the position and the velocity of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The center is specified in center.
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param center (in) "center" object
   @param PV (out) position and the velocity of the "target" object
*/
/*--------------------------------------------------------------------------*/
void calceph(treal JD0, treal time, int target, int center, treal PV[6])
{
    calceph_scompute(JD0, time, target, center, PV);
}

/*--------------------------------------------------------------------------*/
/*! prefetch all data to the memory
  return 0 if an error occurs.
  return 1 if  prefech is done.
 @param eph (inout) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_prefetch(t_calcephbin * eph)
{
    int res = 1;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_prefetch(&eph->data.spkernel);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_prefetch(&eph->data.binary);
            break;

            /* GCOVR_EXCL_START */
        case CALCEPH_unknown:
            res = 0;
            break;

        default:
            fatalerror("Unknown ephemeris type in calceph_prefetch\n");
            res = 0;
            break;
            /* GCOVR_EXCL_STOP */
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return a non-zero value if eph could be accessed by multiple threads 
 @param eph (in) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_isthreadsafe(t_calcephbin * eph)
{
    int res = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
#if HAVE_PTHREAD  || HAVE_WIN32API
            res = calceph_spice_isthreadsafe(&eph->data.spkernel);
#endif
            break;

        case CALCEPH_ebinary:
            res = (eph->data.binary.coefftime_array.prefetch != 0) ? 1 : 0;
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_isthreadsafe\n");
            res = 0;
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return EMRAT from file
  @param p_pbinfile (in) ephemeris file
*/
/*--------------------------------------------------------------------------*/
double calceph_getEMRAT(t_calcephbin * p_pbinfile)
{
    double AU = 0.E0;

    switch (p_pbinfile->etype)
    {
        case CALCEPH_espice:
            AU = calceph_spice_getEMRAT(&p_pbinfile->data.spkernel);
            break;

        case CALCEPH_ebinary:
            AU = calceph_inpop_getEMRAT(&p_pbinfile->data.binary);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_close\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return AU;
}

/*--------------------------------------------------------------------------*/
/*! return AU from file
  @param p_pbinfile (in) ephemeris file
*/
/*--------------------------------------------------------------------------*/
double calceph_getAU(t_calcephbin * p_pbinfile)
{
    double AU = 0.E0;

    switch (p_pbinfile->etype)
    {
        case CALCEPH_espice:
            AU = calceph_spice_getAU(&p_pbinfile->data.spkernel);
            break;

        case CALCEPH_ebinary:
            AU = calceph_inpop_getAU(&p_pbinfile->data.binary);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_close\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return AU;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from file
  return 0 if constant is not found
  return the number of values associated to the constant and set p_pdval
  @param p_pbinfile (in) ephemeris file
  @param name (in) constant name
  @param p_pdval (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstant(t_calcephbin * p_pbinfile, const char *name, double *p_pdval)
{
    return calceph_getconstantvd(p_pbinfile, name, p_pdval, 1);
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from file
  return 0 if constant is not found
  return the number of values associated to the constant and set p_pdval
  @param p_pbinfile (in) ephemeris file
  @param name (in) constant name
  @param p_pdval (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstantsd(t_calcephbin * p_pbinfile, const char *name, double *p_pdval)
{
    return calceph_getconstantvd(p_pbinfile, name, p_pdval, 1);
}

/*--------------------------------------------------------------------------*/
/*! fill the array arrayvalue with the nvalue first values of the specified constant from file
  return 0 if constant is not found
  return the number of values associated to the constant (this value may be larger than nvalue),
  if constant is  found
  @param p_pbinfile (in) ephemeris file
  @param name (in) constant name
  @param arrayvalue (out) array of values of the constant.
    On input, the size of the array is nvalue elements.
    On exit, the min(returned value, nvalue) elements are filled.
  @param nvalue (in) number of elements in the array arrayvalue.
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstantvd(t_calcephbin * p_pbinfile, const char *name, double *arrayvalue, int nvalue)
{
    int res = 0;

    /* return the number of constant if empty array is provided */
    if (nvalue <= 0)
    {
        double aval;

        res = calceph_getconstantvd(p_pbinfile, name, &aval, 1);
    }
    else if (strcmp(name, "AU") == 0)
    {
        *arrayvalue = calceph_getAU(p_pbinfile);
        res = (*arrayvalue != 0.E0) ? 1 : 0;
    }
    else if (strcmp(name, "EMRAT") == 0)
    {
        *arrayvalue = calceph_getEMRAT(p_pbinfile);
        res = (*arrayvalue != 0.E0) ? 1 : 0;
    }
    else
    {
        switch (p_pbinfile->etype)
        {
            case CALCEPH_espice:
                res = calceph_spice_getconstant_vd(&p_pbinfile->data.spkernel, name, arrayvalue, nvalue);
                break;

            case CALCEPH_ebinary:
                res = calceph_inpop_getconstant(&p_pbinfile->data.binary, name, arrayvalue);
                break;

            default:
                /* GCOVR_EXCL_START */
                fatalerror("Unknown ephemeris type in calceph_getconstantvd\n");
                /* GCOVR_EXCL_STOP */
                break;
        }
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from file
  return 0 if constant is not found
  return the number of values associated to the constant and set p_pdval
  @param p_pbinfile (in) ephemeris file
  @param name (in) constant name
  @param p_pdval (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstantss(t_calcephbin * p_pbinfile, const char *name, t_calcephcharvalue value)
{
    t_calcephcharvalue arval[1];

    int res, k;

    for (k = 0; k < CALCEPH_MAX_CONSTANTVALUE; k++)
        arval[0][k] = ' ';
    res = calceph_getconstantvs(p_pbinfile, name, arval, 1);
    if (res)
    {
        for (k = 0; k < CALCEPH_MAX_CONSTANTVALUE; k++)
            value[k] = arval[0][k];
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! fill the array arrayvalue with the nvalue first values of the specified constant from file
  return 0 if constant is not found
  return the number of values associated to the constant (this value may be larger than nvalue),
  if constant is  found
  @param p_pbinfile (in) ephemeris file
  @param name (in) constant name
  @param arrayvalue (out) array of values of the constant.
    On input, the size of the array is nvalue elements.
    On exit, the min(returned value, nvalue) elements are filled.
  @param nvalue (in) number of elements in the array arrayvalue.
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstantvs(t_calcephbin * p_pbinfile, const char *name, t_calcephcharvalue * arrayvalue, int nvalue)
{
    int res = 0, k;

    double dval;

    /* return the number of constant if empty array is provided */
    if (nvalue <= 0)
    {
        t_calcephcharvalue aval;

        res = calceph_getconstantvs(p_pbinfile, name, &aval, 1);
    }
    else if (strcmp(name, "AU") == 0)
    {
        dval = calceph_getAU(p_pbinfile);
        for (k = 0; k < CALCEPH_MAX_CONSTANTVALUE; k++)
            arrayvalue[0][k] = ' ';
        calceph_snprintf(arrayvalue[0], CALCEPH_MAX_CONSTANTVALUE, "%23.16E", dval);
        res = (dval != 0.E0) ? 1 : 0;
    }
    else if (strcmp(name, "EMRAT") == 0)
    {
        dval = calceph_getEMRAT(p_pbinfile);
        for (k = 0; k < CALCEPH_MAX_CONSTANTVALUE; k++)
            arrayvalue[0][k] = ' ';
        calceph_snprintf(arrayvalue[0], CALCEPH_MAX_CONSTANTVALUE, "%23.16E", dval);
        res = (dval != 0.E0) ? 1 : 0;
    }
    else
    {
        switch (p_pbinfile->etype)
        {
            case CALCEPH_espice:
                res = calceph_spice_getconstant_vs(&p_pbinfile->data.spkernel, name, arrayvalue, nvalue);
                break;

            case CALCEPH_ebinary:
                res = calceph_inpop_getconstant(&p_pbinfile->data.binary, name, &dval);
                for (k = 0; k < CALCEPH_MAX_CONSTANTVALUE; k++)
                    arrayvalue[0][k] = ' ';
                calceph_snprintf(arrayvalue[0], CALCEPH_MAX_CONSTANTVALUE, "%23.16E", dval);
                break;

            default:
                /* GCOVR_EXCL_START */
                fatalerror("Unknown ephemeris type in calceph_getconstantvs\n");
                /* GCOVR_EXCL_STOP */
                break;
        }
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return the number of constants available in the ephemeris file

  @param eph (inout) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstantcount(t_calcephbin * eph)
{
    int res = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_getconstantcount(&eph->data.spkernel);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_getconstantcount(&eph->data.binary);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_getconstantcount\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! get the name and its first value of the constant available
    at some index from the ephemeris file
  return the number of values associated to the constant and set value
   return 0 if index isn't valid

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the constant (must be between 1 and calceph_getconstantcount() )
  @param name (out) name of the constant
  @param value (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_getconstantindex(t_calcephbin * eph, int index, char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    int res = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_getconstantindex(&eph->data.spkernel, index, name, value);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_getconstantindex(&eph->data.binary, index, name, value);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_getconstantindex\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/********************************************************/
/*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
    for a given target and center  at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param center (in) "center" object
   @param unit (in) sum of CALCEPH_UNIT_???
   @param PV (out) position and the velocity of the "target" object
*/
/********************************************************/
int calceph_compute_unit(t_calcephbin * eph, double JD0, double time, int target, int center, int unit, double PV[6])
{
    int order = STATETYPE_ORDER_DEFAULTVALUE;

    return calceph_compute_order(eph, JD0, time, target, center, unit, order, PV);
}

/*--------------------------------------------------------------------------*/
/*! Return the orientation of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (in) ephemeris descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or CALCEPH_USE_NAIFID
   @param PV (out) orientation (euler angles and their derivatives) of the "target" object
*/
/*--------------------------------------------------------------------------*/
int calceph_orient_unit(t_calcephbin * eph, double JD0, double time, int target, int unit, double PV[6])
{
    int order = STATETYPE_ORDER_DEFAULTVALUE;

    return calceph_orient_order(eph, JD0, time, target, unit, order, PV);
}

/*--------------------------------------------------------------------------*/
/*! Return the rotational angular momentum (G/mR^2) of the object
  "target" in the vector PV at the specified time
  (time elapsed from JD0).
  The output is expressed according to unit.

  return 0 on error.
  return 1 on success.

   @param eph (in) ephemeris descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object
   @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or CALCEPH_USE_NAIFID
   @param PV (out) rotational angular momentum (G/mR^2) and their derivatives of the "target" object
*/
/*--------------------------------------------------------------------------*/
int calceph_rotangmom_unit(t_calcephbin * eph, double JD0, double time, int target, int unit, double PV[6])
{
    int order = STATETYPE_ORDER_DEFAULTVALUE;

    return calceph_rotangmom_order(eph, JD0, time, target, unit, order, PV);
}

/********************************************************/
/*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
 for a given target and center  at the specified time
 (time elapsed from JD0).
 The output is expressed according to unit.

 return 0 on error.
 return 1 on success.

 @param pephbin (inout) ephemeris file descriptor
 @param JD0 (in) reference time (could be 0)
 @param time (in) time elapsed from JD0
 @param target (in) "target" object
 @param center (in) "center" object
 @param unit (in) sum of CALCEPH_UNIT_???
 @param order (in) order of the computation
 =0 : Positions are  computed
 =1 : Position+Velocity  are computed
 =2 : Position+Velocity+Acceleration  are computed
 =3 : Position+Velocity+Acceleration+Jerk  are computed
 @param PVAJ (out) positions and their derivatives of the "target" object
 This array must be able to contain at least 3*(order+1) floating-point numbers.
 On exit, PVAJ[0:3*(order+1)-1] contain valid floating-point numbers.
 */
/********************************************************/
int calceph_compute_order(t_calcephbin * pephbin, double JD0, double time, int target, int center,
                          int unit, int order, double *PVAJ)
{
    int res;

    switch (pephbin->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_compute_unit(&pephbin->data.spkernel, JD0, time, target, center, unit, order, PVAJ);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_compute_unit(pephbin, JD0, time, target, center, unit, order, PVAJ);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_compute_unit\n");
            res = 0;
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! Return the orientation of the object
 "target" in the vector PV at the specified time
 (time elapsed from JD0).
 The output is expressed according to unit.

 return 0 on error.
 return 1 on success.

 @param eph (in) ephemeris descriptor
 @param JD0 (in) reference time (could be 0)
 @param time (in) time elapsed from JD0
 @param target (in) "target" object
 @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or CALCEPH_USE_NAIFID
 @param order (in) order of the computation
 =0 : orientations are computed
 =1 : orientations and their first derivatives are computed
 =2 : orientations and their first,second derivatives  are computed
 =3 : orientations and their first, second, third derivatives  are computed
 @param PVAJ (out) orientation (euler angles and their derivatives) of the "target" object
 This array must be able to contain at least 3*(order+1) floating-point numbers.
 On exit, PVAJ[0:3*(order+1)-1] contain valid floating-point numbers.
 */
/*--------------------------------------------------------------------------*/
int calceph_orient_order(t_calcephbin * eph, double JD0, double time, int target, int unit, int order, double *PVAJ)
{
    int res;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            res = calceph_spice_orient_unit(&eph->data.spkernel, JD0, time, target, unit, order, PVAJ);
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_orient_unit(eph, JD0, time, target, unit, order, PVAJ);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_orient_unit/calceph_orient_order\n");
            res = 0;
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! Return the rotational angular momentum (G/mR^2) of the object
 "target" in the vector PV at the specified time
 (time elapsed from JD0).
 The output is expressed according to unit.

 return 0 on error.
 return 1 on success.

 @param eph (in) ephemeris descriptor
 @param JD0 (in) reference time (could be 0)
 @param time (in) time elapsed from JD0
 @param target (in) "target" object
 @param unit (in) units of PV ( combination of CALCEPH_UNIT_??? ) and/or CALCEPH_USE_NAIFID
 @param order (in) order of the computation
 =0 : (G/mR^2) are computed
 =1 : (G/mR^2) and their first derivatives are computed
 =2 : (G/mR^2) and their first,second derivatives  are computed
 =3 : (G/mR^2) and their first, second, third derivatives  are computed
 @param PVAJ (out) rotational angular momentum (G/mR^2) and their derivatives of the "target" object
 This array must be able to contain at least 3*(order+1) floating-point numbers.
 On exit, PVAJ[0:3*(order+1)-1] contain valid floating-point numbers.
 */
/*--------------------------------------------------------------------------*/
int calceph_rotangmom_order(t_calcephbin * eph, double JD0, double time, int target, int unit, int order, double *PVAJ)
{
    int res;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            fatalerror("Rotational angular momentum (G/(mR^2)) is not available in this ephemeris file\n");
            res = 0;
            break;

        case CALCEPH_ebinary:
            res = calceph_inpop_rotangmom_unit(eph, JD0, time, target, unit, order, PVAJ);
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_rotangmom_unit/calceph_rotangmom_order\n");
            res = 0;
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*!
     This function converts the time of the quantities, which depends on time in the state vector
     according to the units in input and output.
       to the OutputUnit as output.

  it returns 0 if the function fails to convert.

  @param InputUnit  (in) output unit of the statetype
  @param OutputUnit  (in) output unit of the statetype
  @param Planet (inout) position and velocity
          Input : expressed in InputUnit units.
          Output : expressed in OutputUnit units.
*/
/*--------------------------------------------------------------------------*/
int calceph_unit_convert_quantities_time(stateType * Planet, int InputUnit, int OutputUnit)
{
    int res = 1;

    /* some checks */
    if ((OutputUnit & (CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)) == 0)
    {
        res = 0;
        fatalerror("Units must include CALCEPH_UNIT_DAY or CALCEPH_UNIT_SEC \n");
    }
    if ((OutputUnit & (CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)) == CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)
    {
        res = 0;
        fatalerror("Units must include CALCEPH_UNIT_DAY or CALCEPH_UNIT_SEC \n");
    }
    /* performs the conversion */
    if ((OutputUnit & CALCEPH_UNIT_DAY) == CALCEPH_UNIT_DAY && (InputUnit & CALCEPH_UNIT_SEC) == CALCEPH_UNIT_SEC)
    {
        calceph_stateType_mul_time(Planet, 86400E0);
    }
    if ((InputUnit & CALCEPH_UNIT_DAY) == CALCEPH_UNIT_DAY && (OutputUnit & CALCEPH_UNIT_SEC) == CALCEPH_UNIT_SEC)
    {
        calceph_stateType_div_time(Planet, 86400E0);
    }

    return res;
}

/*--------------------------------------------------------------------------*/
/*! return, for a given type of segment, the maximal order of the derivatives
  It returns -1 if the segment type is unknown or not supported by the library

  @param idseg (in) number of the segment
*/
/*--------------------------------------------------------------------------*/
int calceph_getmaxsupportedorder(int idseg)
{
    int res = -1;

    switch (idseg)
    {
        case CALCEPH_SEGTYPE_ORIG_0:
        case CALCEPH_SEGTYPE_SPK_2:
        case CALCEPH_SEGTYPE_SPK_3:
        case CALCEPH_SEGTYPE_SPK_8:
        case CALCEPH_SEGTYPE_SPK_9:
        case CALCEPH_SEGTYPE_SPK_12:
        case CALCEPH_SEGTYPE_SPK_13:
        case CALCEPH_SEGTYPE_SPK_14:
        case CALCEPH_SEGTYPE_SPK_18:
        case CALCEPH_SEGTYPE_SPK_19:
        case CALCEPH_SEGTYPE_SPK_20:
        case CALCEPH_SEGTYPE_SPK_102:
        case CALCEPH_SEGTYPE_SPK_103:
        case CALCEPH_SEGTYPE_SPK_120:
            res = 3;
            break;

        case CALCEPH_SEGTYPE_SPK_1:
        case CALCEPH_SEGTYPE_SPK_5:
        case CALCEPH_SEGTYPE_SPK_17:
        case CALCEPH_SEGTYPE_SPK_21:
            res = 1;
            break;

        default:
            break;
    }
    return res;
}
