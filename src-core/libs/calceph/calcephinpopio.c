/*-----------------------------------------------------------------*/
/*! 
  \file calcephinpopio.c 
  \brief perform I/O on INPOP/original JPL ephemeris data file 
         interpolate INPOP and JPL Ephemeris data.
         compute position and velocity from binary ephemeris file.

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2008-2024, CNRS
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
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"
#include "util.h"
#include "calcephchebyshev.h"

/*==========================================================================*/
/* private functions                                                        */
/*==========================================================================*/
static int calceph_interpol_PV_planet(struct calcephbin_inpop *p_pbinfile, treal TimeJD0, treal Timediff,
                                      int Target, int *Unit, stateType * Planet);
static int calceph_inpop_unit_convert(const struct calcephbin_inpop *pephbin, stateType * Planet,
                                      int Target, int InputUnit, int OutputUnit);
static void calceph_inpop_prefetch_update_cfta(const struct calcephbin_inpop *eph, t_memarcoeff * cfta);
static int calceph_inpop_open_extension_inpop(FILE * file, const char *fileName, struct calcephbin_inpop *res);
static int calceph_inpop_open_extension_jpl(FILE * file, const char *fileName, struct calcephbin_inpop *res);

/*--------------------------------------------------------------------------*/
/*!                                                                       
     This function is used by the functions below to read an array of     
     Tchebeychev coefficients from a binary ephemeris data file.          
  @return return 1 if no error occurs. return 0 if an error occurs. 
  
  @param pcoeftime_array (inout) coefficient array + binary file
  @param Time (in) Desired record time.   
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_readcoeff(t_memarcoeff * pcoeftime_array, double Time)
{
    if (pcoeftime_array->prefetch == 0)
    {
        int n = (int) fread(pcoeftime_array->Coeff_Array, sizeof(double), pcoeftime_array->ARRAY_SIZE,
                            pcoeftime_array->file);

        if (n != pcoeftime_array->ARRAY_SIZE)
        {
            buffer_error_t buffer_error;

            fatalerror("Can't read ephemeris file at time=%g. System error: '%s'\n", Time,
                       calceph_strerror_errno(buffer_error));
            return 0;
        }
        /* swap the data if necessary */
        if (pcoeftime_array->swapbyteorder)
            swapdblarray(pcoeftime_array->Coeff_Array, pcoeftime_array->ARRAY_SIZE);
    }

    pcoeftime_array->T_beg = pcoeftime_array->Coeff_Array[0];
    pcoeftime_array->T_end = pcoeftime_array->Coeff_Array[1];
    pcoeftime_array->T_span = pcoeftime_array->T_end - pcoeftime_array->T_beg;

    /* verifie s'il y a eu une erreur de fread ou de fseek */
    if (Time < pcoeftime_array->T_beg || Time > pcoeftime_array->T_end)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Read a bad block [%.16g , %.16g ] in the ephemeris file at the time %g\n",
                   pcoeftime_array->T_beg, pcoeftime_array->T_end, Time);
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    return 1;
}

/*--------------------------------------------------------------------------*/
/*!                                                                       
     Seek the record containing the time.
     Read an array of Tchebychev coefficients from a binary ephemeris data file.   
     
  @return return true if time is available 
  
  @param pephbin (inout) coefficient array + binary file
  @param Time (in) Desired record time.   
  
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_seekreadcoeff(t_memarcoeff * pephbin, double Time)
{

    double T_delta = 0.0;

    off_t Offset = 0;

    /*--------------------------------------------------------------------------*/
    /*  Find ephemeris data that record contains input time. Note that one, and */
    /*  only one, of the following conditional statements will be true (if both */
    /*  were false, this function would not have been called).                  */
    /*--------------------------------------------------------------------------*/

    if (Time < pephbin->T_beg)  /* Compute backwards location offset */
    {
        T_delta = pephbin->T_beg - Time;
        Offset = (int) -ceil(T_delta / pephbin->T_span);
    }

    if (Time > pephbin->T_end)  /* Compute forwards location offset */
    {
        T_delta = Time - pephbin->T_end;
        Offset = (int) ceil(T_delta / pephbin->T_span);
    }

    /*--------------------------------------------------------------------------*/
    /*  Retrieve ephemeris data from new record.                                */
    /*--------------------------------------------------------------------------*/
    pephbin->offfile += Offset * pephbin->ARRAY_SIZE * sizeof(double);

    if (pephbin->prefetch == 0)
    {
        if (fseeko(pephbin->file, pephbin->offfile, SEEK_SET) != 0)
        {
            buffer_error_t buffer_error;

            fatalerror("Can't jump in the file at time=%g. System error: '%s'\n", Time,
                       calceph_strerror_errno(buffer_error));
            return 0;
        }
#if DEBUG
        if (ftello(pephbin->file) != pephbin->offfile)
        {
            printf("error in the file location %lu %lu\n",
                   (unsigned long) pephbin->offfile, (unsigned long) ftello(pephbin->file));
        }
        printf("set to the location %lu\n", (unsigned long) pephbin->offfile);
#endif
    }
    else
    {                           /* prefect was done before */
        pephbin->Coeff_Array = pephbin->mmap_buffer + pephbin->offfile / sizeof(double);
    }
    return calceph_inpop_readcoeff(pephbin, Time);
}

/*--------------------------------------------------------------------------*/
/*!  Read and fill only the additional data of the INPOP files
     
  @return it returns  0 on failure
  
  @param file (inout) file descriptor of an ephemeris data file.   
  @param fileName (in) A character string giving the name of an ephemeris data file.   
  @param res (inout) ephemeris descriptor filled with data form the file.   
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_open_extension_inpop(FILE * file, const char *fileName, struct calcephbin_inpop *res)
{
    t_HeaderBlockExtensionInpop headerblockinpop;   /* specific inpop files */
    int j;

    /* read the specific part */
    if (fread(&headerblockinpop, sizeof(t_HeaderBlockExtensionInpop), 1, file) != 1)
    {
        buffer_error_t buffer_error;

        fatalerror
            ("Can't read the additional inpop information header block from the ephemeris file '%s'\nSystem error : '%s'\n",
             fileName, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }
    res->H1.recordsize = headerblockinpop.recordsize;
    for (j = 0; j < 3; j++)
    {
        res->H1.TTmTDBPtr[j] = headerblockinpop.TTmTDBPtr[j];
    }
    return 1;
}

/*--------------------------------------------------------------------------*/
/*!  Read and fill only the additional data of the JPL files
     
  @return it returns  0 on failure
  
  @param file (inout) file descriptor of an ephemeris data file.   
  @param fileName (in) A character string giving the name of an ephemeris data file.   
  @param res (inout) ephemeris descriptor filled with data form the file.   
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_open_extension_jpl(FILE * file, const char *fileName, struct calcephbin_inpop *res)
{
    buffer_error_t buffer_error;
    t_HeaderBlockExtensionJPL headerblockjpl;   /* specific inpop files */
    int j;
    int nConst = res->H1.numConst;

    /* read the additional constant */
    if (res->coefftime_array.swapbyteorder)
        nConst = swapint(nConst);

    /* additional check the validity of nConst */
    if ((nConst < 0) || (nConst > 3000))
    {
        fatalerror
            ("Can't read the additional JPL constants header block from the ephemeris file (Number of constants=%d).\n",
             nConst);
        calceph_inpop_close(res);
        return 0;
    }

    if (nConst > 400)
    {
        size_t nreadbytes = 6 * (nConst - 400);

        if (fread(res->H1.constName[400], sizeof(char), nreadbytes, file) != nreadbytes)
        {
            fatalerror
                ("Can't read the additional JPL constants header block from the ephemeris file '%s'\nSystem error : '%s'\n",
                 fileName, calceph_strerror_errno(buffer_error));
            calceph_inpop_close(res);
            return 0;
        }
    }

    /* read the specific part */
    if (fread(&headerblockjpl, sizeof(t_HeaderBlockExtensionJPL), 1, file) != 1)
    {
        fatalerror
            ("Can't read the additional JPL information header block from the ephemeris file '%s'\nSystem error : '%s'\n",
             fileName, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }
    for (j = 0; j < 3; j++)
    {
        res->H1.lunarAngularvelocityPtr[j] = headerblockjpl.lunarAngularvelocityPtr[j];
        res->H1.TTmTDBPtr[j] = headerblockjpl.TTmTDBPtr[j];
    }

    return 1;
}

/*--------------------------------------------------------------------------*/
/*!  
     Open the "filename" ephemeris file .
     Read the header data block
     Read the constant data block
     Read the first coefficient data bock.
     
  @return descriptor of the ephemeris and the first array of coefficients.  
  it returns  0 on failure
  
  @param file (inout) file descriptor of an ephemeris data file.   
  @param fileName (in) A character string giving the name of an ephemeris data file.   
  @param res (out) ephemeris descriptor filled with data form the file.   
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_open(FILE * file, const char *fileName, struct calcephbin_inpop *res)
{
    buffer_error_t buffer_error;
    t_HeaderBlockBase headerblockbase;  /* common part of jpl or inpop files */

    int computerecordsize = 0;  /* = 1 => require to compute the record size */

    int j, k;

    int newdenum;

    res->coefftime_array.Coeff_Array = NULL;
    res->coefftime_array.file = file;
    res->coefftime_array.mmap_used = 0;
    res->coefftime_array.mmap_buffer = NULL;
    res->coefftime_array.mmap_size = 0;
    res->coefftime_array.prefetch = 0;
    res->timescale = etimescale_TDB;

    /*--------------------------------------------------------------------------*/
    /*  Intialize the asteroid information to empty .                           */
    /*--------------------------------------------------------------------------*/
    calceph_empty_asteroid(&res->asteroids);

    /*--------------------------------------------------------------------------*/
    /*  Intialize the Planetary Angular Momentum information to empty .        */
    /*--------------------------------------------------------------------------*/
    calceph_empty_pam(&res->pam);

    /*--------------------------------------------------------------------------*/
    /*  Read header & first coefficient array, then return status code.         */
    /*--------------------------------------------------------------------------*/
    if (fseeko(file, 0, SEEK_SET) != 0)
    {
        fatalerror("Can't jump to the beginning of the file '%s'. System error: '%s'\n", fileName,
                   calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }

    /* Read first three header records from ephemeris file */

    /* read the common part of JPL and INPOP files */
    if (fread(&headerblockbase, sizeof(t_HeaderBlockBase), 1, file) != 1)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read header block from the ephemeris file '%s'\nSystem error : '%s'\n",
                   fileName, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    /* copy the common part headerblockbase to H1 */
    res->H1.numConst = headerblockbase.numConst;
    res->H1.AU = headerblockbase.AU;
    res->H1.EMRAT = headerblockbase.EMRAT;
    res->H1.DENUM = headerblockbase.DENUM;
    for (j = 0; j < 400; j++)
    {
        for (k = 0; k < 6; k++)
            res->H1.constName[j][k] = headerblockbase.constName[j][k];
    }
    for (j = 0; j < 3; j++)
    {
        for (k = 0; k < 84; k++)
        {
            res->H1.label[j][k] = headerblockbase.label[j][k];
        }
        res->H1.timeData[j] = headerblockbase.timeData[j];
        for (k = 0; k < 12; k++)
        {
            res->H1.coeffPtr[k][j] = headerblockbase.coeffPtr[k][j];
        }
        res->H1.libratPtr[j] = headerblockbase.libratPtr[j];
    }

    res->coefftime_array.ncomp = 3;
    res->coefftime_array.ncompTTmTDB = 3;
    res->coefftime_array.swapbyteorder = false;
    res->isinAU = false;
    res->isinDay = true;
    res->haveTTmTDB = false;
    res->haveNutation = false;
    res->haveLunarAngularVelocity = false;

    /* load the specific parts */
    switch (res->H1.DENUM)
    {
        case EPHEMERIS_INPOP:
            if (calceph_inpop_open_extension_inpop(file, fileName, res) == 0)
                return 0;
            res->coefftime_array.ARRAY_SIZE = res->H1.recordsize;
            res->coefftime_array.ncomp = 6;
            res->coefftime_array.ncompTTmTDB = 6;
            break;
        default:
            switch (swapint(res->H1.DENUM))
            {
                case EPHEMERIS_INPOP:
                    if (calceph_inpop_open_extension_inpop(file, fileName, res) == 0)
                        return 0;
                    res->coefftime_array.swapbyteorder = true;
                    res->coefftime_array.ARRAY_SIZE = res->H1.recordsize;
                    if (res->coefftime_array.swapbyteorder)
                        res->coefftime_array.ARRAY_SIZE = swapint(res->coefftime_array.ARRAY_SIZE);
                    res->coefftime_array.ncomp = 6;
                    res->coefftime_array.ncompTTmTDB = 6;
                    break;

                default:
                    res->coefftime_array.ARRAY_SIZE = 0;
                    res->coefftime_array.ncompTTmTDB = 1;
                    newdenum = res->H1.DENUM;
                    if (((unsigned int) res->H1.DENUM) > (1U << 15))
                    {
                        newdenum = swapint(res->H1.DENUM);
                        res->coefftime_array.swapbyteorder = true;
                        computerecordsize = 1;
                    }
                    if (calceph_inpop_open_extension_jpl(file, fileName, res) == 0)
                        return 0;
                    if (newdenum >= EPHEMERIS_DE200)
                    {
                        computerecordsize = 1;
                    }
                    else
                    {
                        fatalerror("Opened wrong file: '%s' for ephemeris: DENUM=%d.\n", fileName, newdenum);
                        calceph_inpop_close(res);
                        return 0;
                    }
                    break;
            }
            break;
    }

    /*--------------------------------------------------------------------------*/
    /* swap the header if necessary */
    /*--------------------------------------------------------------------------*/
    if (res->coefftime_array.swapbyteorder)
    {
        swapdblarray(res->H1.timeData, 3);
        res->H1.numConst = swapint(res->H1.numConst);
        res->H1.AU = swapdbl(res->H1.AU);
        res->H1.EMRAT = swapdbl(res->H1.EMRAT);
        swapintarray((int *) res->H1.coeffPtr, 36);
        res->H1.DENUM = swapint(res->H1.DENUM);
        swapintarray(res->H1.libratPtr, 3);
        swapintarray(res->H1.lunarAngularvelocityPtr, 3);
        swapintarray(res->H1.TTmTDBPtr, 3);
        res->H1.recordsize = swapint(res->H1.recordsize);
    }

    /*--------------------------------------------------------------------------*/
    /* compute record size if required */
    /*--------------------------------------------------------------------------*/
    if (computerecordsize == 1)
    {
        int failure = 0;

        if (res->coefftime_array.ncomp <= 0)
            failure = 1;
        for (j = 0; j < 12; j++)
        {
            int ncompcur = (j == 11) ? 2 : res->coefftime_array.ncomp;

            if (res->H1.coeffPtr[j][1] < 0 || res->H1.coeffPtr[j][2] < 0)
                failure = 1;
            res->coefftime_array.ARRAY_SIZE += res->H1.coeffPtr[j][1] * res->H1.coeffPtr[j][2] * ncompcur;
        }
        /* nutation part */
        res->haveNutation = (res->H1.coeffPtr[11][0] > 0 && res->H1.coeffPtr[11][1] * res->H1.coeffPtr[11][2] != 0);

        /* libration part */
        if (res->H1.libratPtr[1] < 0 || res->H1.libratPtr[2] < 0)
            failure = 1;
        res->coefftime_array.ARRAY_SIZE += res->H1.libratPtr[1] * res->H1.libratPtr[2] * res->coefftime_array.ncomp;
        res->coefftime_array.ARRAY_SIZE += 2;

        /* Lunar Angular velocity part */
        res->haveLunarAngularVelocity = (res->H1.lunarAngularvelocityPtr[0] > 0 &&
                                         res->H1.lunarAngularvelocityPtr[1] > 0 &&
                                         res->H1.lunarAngularvelocityPtr[2] > 0);

        if (res->haveLunarAngularVelocity == true)
        {
            if (res->H1.lunarAngularvelocityPtr[1] < 0 || res->H1.lunarAngularvelocityPtr[2] < 0)
                failure = 1;
            res->coefftime_array.ARRAY_SIZE +=
                res->H1.lunarAngularvelocityPtr[1] * res->H1.lunarAngularvelocityPtr[2] * res->coefftime_array.ncomp;
        }

        /* TT-TDB part */
        res->haveTTmTDB = (res->H1.TTmTDBPtr[0] > 0 && res->H1.TTmTDBPtr[1] > 0 && res->H1.TTmTDBPtr[2] > 0);
        if (res->haveTTmTDB == true)
        {
            if (res->H1.TTmTDBPtr[1] < 0 || res->H1.TTmTDBPtr[2] < 0)
                failure = 1;
            res->coefftime_array.ARRAY_SIZE +=
                res->H1.TTmTDBPtr[1] * res->H1.TTmTDBPtr[2] * res->coefftime_array.ncompTTmTDB;
        }

        res->H1.recordsize = res->coefftime_array.ARRAY_SIZE;

        if (failure || res->coefftime_array.ARRAY_SIZE < 0)
        {
            fatalerror("Opened wrong file: '%s' for ephemeris: DENUM=%d.\n", fileName, res->H1.DENUM);
            calceph_inpop_close(res);
            return 0;
        }
    }

    /*--------------------------------------------------------------------------*/
    /* read constants */
    /*--------------------------------------------------------------------------*/
    res->coefftime_array.Coeff_Array = (double *) malloc(sizeof(double) * res->coefftime_array.ARRAY_SIZE);
    if (res->coefftime_array.Coeff_Array == NULL)
    {
        fatalerror("Can't allocate memory for %d reals\nSystem error : '%s'\n",
                   res->coefftime_array.ARRAY_SIZE, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }
    /*lecture des constantes */
    /*se positionne au debut du deuxieme record */
    if (fseeko(file, sizeof(double) * res->coefftime_array.ARRAY_SIZE, SEEK_SET) != 0)
    {
        fatalerror("Can't jump to constant values in the ephemeris file '%s'\nSystem error : '%s'\n",
                   fileName, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }
    if (fread(res->constVal, sizeof(double), res->H1.numConst, file) != (size_t) res->H1.numConst)
    {
        fatalerror("Can't read %d constant values from the ephemeris file '%s'\nSystem error : '%s'\n",
                   res->H1.numConst, fileName, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }
    /* swap the data if necessary */
    if (res->coefftime_array.swapbyteorder)
        swapdblarray(res->constVal, res->H1.numConst);

    /* ajuste le nombre de composantes pour inpop */
    if (res->H1.DENUM == EPHEMERIS_INPOP)
    {
        double dformat = 0.E0;

        double dunite;

        double dtimescale;

        if (calceph_inpop_getconstant(res, "FORMAT", &dformat))
        {
            double SS[3];

            int locnextrecord;

            for (j = 0; j <= 2; j++)
                SS[j] = res->H1.timeData[j];

            if (((int) (fmod(dformat, 10))) == 1)
            {
                res->coefftime_array.ncomp = 3;
                res->coefftime_array.ncompTTmTDB = 3;
            };
            if (((int) (fmod(rint(dformat / 10), 10))) == 1)
            {
                res->haveTTmTDB = true;
            };

            locnextrecord = 3 + (off_t) (SS[1] - SS[0]) / SS[2];
            /*--------------------------------------------------------------------------*/
            /*  initialize the asteroids data */
            /*--------------------------------------------------------------------------*/
            if (((int) (fmod(rint(dformat / 100), 10))) == 1)
            {
                if (calceph_init_asteroid
                    (&res->asteroids, res->coefftime_array.file, res->coefftime_array.swapbyteorder, SS,
                     res->H1.recordsize, res->coefftime_array.ncomp, &locnextrecord) == 0)
                {
                    calceph_inpop_close(res);
                    return 0;
                }
            }

            /*--------------------------------------------------------------------------*/
            /*  initialize the Planetary Angular Momentum data */
            /*--------------------------------------------------------------------------*/
            if (((int) (fmod(rint(dformat / 1000), 10))) == 1)
            {
                if (calceph_init_pam(&res->pam, res->coefftime_array.file, res->coefftime_array.swapbyteorder, SS,
                                     res->H1.recordsize, &locnextrecord) == 0)
                {
                    calceph_inpop_close(res);
                    return 0;
                }
            }
        }

        if (calceph_inpop_getconstant(res, "UNITE", &dunite))
        {
            int iunite = (int) dunite;

            res->isinAU = (iunite == 0);
            res->isinDay = (iunite == 0 || iunite == 1);
        }
        else
        {
            fatalerror("Can't find constant 'UNITE' in ephemeris file. Wrong version of ephemeris file\n");
            calceph_inpop_close(res);
            return 0;
        }

        /*--------------------------------------------------------------------------*/
        /*  initialize the time scale */
        /*--------------------------------------------------------------------------*/
        if (calceph_inpop_getconstant(res, "TIMESC", &dtimescale))
        {
            res->timescale = (t_etimescale) dtimescale;
        }
    }

    /*--------------------------------------------------------------------------*/
    /*  read the third record : first records for the planets */
    /*--------------------------------------------------------------------------*/
    res->coefftime_array.offfile = 2 * sizeof(double) * res->coefftime_array.ARRAY_SIZE;
    if (fseeko(file, res->coefftime_array.offfile, SEEK_SET))
    {
        fatalerror("Can't jump to first data record in the ephemeris file '%s'\nSystem error : '%s'\n",
                   fileName, calceph_strerror_errno(buffer_error));
        calceph_inpop_close(res);
        return 0;
    }
    if (!calceph_inpop_readcoeff(&res->coefftime_array, res->H1.timeData[0]))
    {
        calceph_inpop_close(res);
        return 0;
    }

#if DEBUG
    {
        printf("\n  calceph_open : \n");
        printf("\n      ARRAY_SIZE = %4d", res->coefftime_array.ARRAY_SIZE);
        printf("\n      T_Beg      = %7.3f", res->coefftime_array.T_beg);
        printf("\n      T_End      = %7.3f", res->coefftime_array.T_end);
        printf("\n      T_Span     = %7.3f", res->coefftime_array.T_span);
        printf("\n      Number of component     = %d", res->coefftime_array.ncomp);
        printf("\n      isinAU     = %d", res->isinAU);
        printf("\n      timescale     = %d", res->isinAU);
        printf("\n      TT-TDB     = %d", res->timescale);
        printf("\n      record size     = %d\n\n", res->H1.recordsize);
    }
#endif /*DEBUG*/
        return 1;
}

/*--------------------------------------------------------------------------*/
/*! Close the  ephemeris file and destroy the ephemeris descriptor
 @param eph (inout) ephemeris descriptor to be destroyed
*/
/*--------------------------------------------------------------------------*/
void calceph_inpop_close(struct calcephbin_inpop *eph)
{
    if (eph->coefftime_array.file != NULL)
        fclose(eph->coefftime_array.file);
    if (eph->coefftime_array.prefetch == 0)
    {
        if (eph->coefftime_array.Coeff_Array != NULL)
            free(eph->coefftime_array.Coeff_Array);
    }
    eph->coefftime_array.file = NULL;
    eph->coefftime_array.Coeff_Array = NULL;
    if (eph->coefftime_array.mmap_buffer)
    {
#if HAVE_MMAP
        if (eph->coefftime_array.mmap_used)
        {
            munmap(eph->coefftime_array.mmap_buffer, eph->coefftime_array.mmap_size);
        }
        else
#endif
        {
            free(eph->coefftime_array.mmap_buffer);
        }
    }
    calceph_free_asteroid(&eph->asteroids);
    calceph_free_pam(&eph->pam);
}

/*--------------------------------------------------------------------------*/
/*! Prefetch for reading any INPOP file
     
  @return  1 on sucess and 0 on failure
  
  @param pcoeftime_array (inout) descriptor of the ephemeris.  
*/
/*--------------------------------------------------------------------------*/
static int calceph_inpop_prefetch_memarcoeff(t_memarcoeff * pcoeftime_array)
{
    int res = 1;

    off_t newlen;

    if (pcoeftime_array->prefetch == 0)
    {
        if (fseeko(pcoeftime_array->file, 0, SEEK_END) != 0)
        {
            return 0;
        }
        newlen = ftello(pcoeftime_array->file);
        if (newlen == -1)
        {
            return 0;
        }

        if (fseeko(pcoeftime_array->file, 0, SEEK_SET) != 0)
        {
            return 0;
        }

        /* swap the data if necessary => do not use mmap */
#if HAVE_MMAP
        if (!pcoeftime_array->swapbyteorder)
        {
#ifdef MAP_POPULATE
            pcoeftime_array->mmap_buffer = (double *) mmap(NULL, (size_t) newlen, PROT_READ, MAP_PRIVATE | MAP_POPULATE,
                                                           fileno(pcoeftime_array->file), 0);
#else
            pcoeftime_array->mmap_buffer =
                (double *) mmap(NULL, (size_t) newlen, PROT_READ, MAP_PRIVATE, fileno(pcoeftime_array->file), 0);
#endif
            if (pcoeftime_array->mmap_buffer == MAP_FAILED)
            {
                pcoeftime_array->mmap_buffer = NULL;
                return 0;
            }
            pcoeftime_array->mmap_size = (size_t) newlen;
            pcoeftime_array->mmap_used = 1;

        }
        else
#endif
        {
            pcoeftime_array->mmap_used = 0;
            if (pcoeftime_array->mmap_size < (size_t) newlen)
            {
                pcoeftime_array->mmap_buffer = (double *) realloc(pcoeftime_array->mmap_buffer, (size_t) newlen);
                if (pcoeftime_array->mmap_buffer == NULL)
                {
                    return 0;
                }
            }

            if (fread(pcoeftime_array->mmap_buffer, sizeof(char), (size_t) newlen, pcoeftime_array->file) !=
                (size_t) newlen)
            {
                free(pcoeftime_array->mmap_buffer);
                pcoeftime_array->mmap_buffer = NULL;
                pcoeftime_array->mmap_size = 0;
                return 0;
            }
            pcoeftime_array->mmap_size = (size_t) newlen;
            /* swap the data if necessary */
            if (pcoeftime_array->swapbyteorder)
                swapdblarray(pcoeftime_array->mmap_buffer, pcoeftime_array->mmap_size / sizeof(double));
        }
        free(pcoeftime_array->Coeff_Array);
        pcoeftime_array->Coeff_Array = pcoeftime_array->mmap_buffer + pcoeftime_array->offfile / sizeof(double);
        pcoeftime_array->T_beg = pcoeftime_array->Coeff_Array[0];
        pcoeftime_array->T_end = pcoeftime_array->Coeff_Array[1];
        pcoeftime_array->T_span = pcoeftime_array->T_end - pcoeftime_array->T_beg;

        pcoeftime_array->prefetch = 1;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! Update the data structure cfta for the prefetching using eph.
     
  @param eph (in) descriptor of the ephemeris.  
  @param cfta (inout) data structure to update.  
*/
/*--------------------------------------------------------------------------*/
static void calceph_inpop_prefetch_update_cfta(const struct calcephbin_inpop *eph, t_memarcoeff * cfta)
{
    cfta->mmap_buffer = eph->coefftime_array.mmap_buffer;
    cfta->mmap_size = 0;
    cfta->prefetch = 1;
    if (cfta->Coeff_Array)
    {
        free(cfta->Coeff_Array);
        cfta->Coeff_Array = cfta->mmap_buffer + cfta->offfile / sizeof(double);
        cfta->T_beg = cfta->Coeff_Array[0];
        cfta->T_end = cfta->Coeff_Array[1];
        cfta->T_span = cfta->T_end - cfta->T_beg;
    }
}

/*--------------------------------------------------------------------------*/
/*! Prefetch for reading any INPOP file
     
  @return  1 on sucess and 0 on failure
  
  @param eph (inout) descriptor of the ephemeris.  
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_prefetch(struct calcephbin_inpop *eph)
{
    int res = calceph_inpop_prefetch_memarcoeff(&eph->coefftime_array);

    if (res == 1)
    {
        /* for asteroids */
        calceph_inpop_prefetch_update_cfta(eph, &eph->asteroids.coefftime_array);

        /* for planetary angular momentum */
        calceph_inpop_prefetch_update_cfta(eph, &eph->pam.coefftime_array);
    }

    return res;
}

/********************************************************/
/*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
    for a given target and center  at the specified time 
  (time elapsed from JD0).
  The output is expressed according to unit.
  The target and center are expressed in the old numbering system
 
  return 0 on error.
  return 1 on success.

   @param pephbin (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object (old numbering system)
   @param center (in) "center" object (old numbering system) 
   @param unit (in) sum of CALCEPH_UNIT_???
   @param order (in) order of the computation
            =0 : Positions are  computed
            =1 : Position+Velocity  are computed
            =2 : Position+Velocity+Acceleration  are computed
            =3 : Position+Velocity+Acceleration+Jerk  are computed
   @param PV (out) quantities of the "target" object
     PV[0:3*(order+1)-1] are valid
*/
/********************************************************/
static int calceph_inpop_compute_unit_oldid(t_calcephbin * pephbin, double JD0, double time, int target,
                                            int center, int unit, int order, double PV[ /*<=12 */ ])
{
    stateType statetarget, statecenter, statelune;

    int detarget, decenter, delune;

    double EMRAT = 1E100;

    int initEMRAT = 0;

    int res = -1;

    if (target == center)
    {
        calceph_PV_set_0(PV, order);
        return 1;
    }

    target--;
    center--;

    statetarget.order = statecenter.order = statelune.order = order;
    delune = MOON;
    /* cas lune ou terre : toujours recuperer la lune */
    if (target == MOON || center == MOON || center == EARTH || target == EARTH)
    {
        res =
            calceph_inpop_interpol_PV(&pephbin->data.binary, JD0, time, delune, -EARTH /* earth */ , unit, &statelune);
    }

    /* cas vecteur Terre-Lune */
    if (center == EARTH && target == MOON)
    {
        calceph_PV_set_stateType(PV, statelune);
    }
    /* cas vecteur Lune-Terre */
    else if (target == EARTH && center == MOON)
    {
        calceph_PV_neg_stateType(PV, statelune);
    }
    /*cas libration  ou TT-TDB or nutations */
    else if ((target == NUTATIONS || target == LIBRATIONS || target == TTMTDB || target == TCGMTCB) && center == -1)
    {
        res = calceph_inpop_interpol_PV(&pephbin->data.binary, JD0, time, target, -1, unit, &statetarget);
        calceph_PV_set_stateType(PV, statetarget);
    }
    /* autre cas */
    else
    {
        detarget = target;
        decenter = center;
        if (target == EM_BARY || target == MOON)
            detarget = EARTH;
        if (decenter == EM_BARY || center == MOON)
            decenter = EARTH;

        calceph_PV_set_0(PV, order);

        if (detarget != SS_BARY && detarget != -1)
        {
            res = calceph_inpop_interpol_PV(&pephbin->data.binary, JD0, time, detarget, SS_BARY, unit, &statetarget);
            calceph_PV_set_stateType(PV, statetarget);
            /* cas target = MOON */
            if (target == MOON)
            {
                if (initEMRAT == 0)
                {
                    EMRAT = calceph_getEMRAT(pephbin);
                    if (EMRAT == 0.E0)
                    {
                        res = 0;
                        fatalerror("Can't find the constant EMRAT\n");
                    }
                    initEMRAT = 1;
                }

                calceph_PV_fma_stateType(PV, EMRAT / (REALC(1.0E0) + EMRAT), statelune);
            }
            /* cas target == EARTH */
            if (target == EARTH)
            {
                if (initEMRAT == 0)
                {
                    EMRAT = calceph_getEMRAT(pephbin);
                    if (EMRAT == 0.E0)
                    {
                        res = 0;
                        fatalerror("Can't find the constant EMRAT\n");
                    }
                    initEMRAT = 1;
                }

                calceph_PV_fms_stateType(PV, REALC(1.0E0) / (REALC(1.0E0) + EMRAT), statelune);
            }
        }

        if (decenter != SS_BARY && decenter != -1)
        {
            res = calceph_inpop_interpol_PV(&pephbin->data.binary, JD0, time, decenter, SS_BARY, unit, &statecenter);
            calceph_PV_sub_stateType(PV, statecenter);

            /* cas center = MOON */
            if (center == MOON)
            {
                if (initEMRAT == 0)
                {
                    EMRAT = calceph_getEMRAT(pephbin);
                    if (EMRAT == 0.E0)
                    {
                        res = 0;
                        fatalerror("Can't find the constant EMRAT\n");
                    }
                    initEMRAT = 1;
                }
                calceph_PV_fms_stateType(PV, EMRAT / (REALC(1.0E0) + EMRAT), statelune);
            }
            /* cas center == EARTH */
            if (center == EARTH)
            {
                if (initEMRAT == 0)
                {
                    EMRAT = calceph_getEMRAT(pephbin);
                    if (EMRAT == 0.E0)
                    {
                        res = 0;
                        fatalerror("Can't find the constant EMRAT\n");
                    }
                    initEMRAT = 1;
                }
                calceph_PV_fma_stateType(PV, REALC(1.0E0) / (REALC(1.0E0) + EMRAT), statelune);
            }
        }
    }

    if (res == -1)
    {
        fatalerror("target=%d or center=%d is invalid\n", target + 1, center + 1);
        res = 0;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! convert an NAIF ID calceph to internal id (old numbering system)
   e.g., calceph_spice_convertid(eph, NAIFID_EARTH_MOON_BARYCENTER) return 13
    return -1 if not supported.
  @param eph (inout) ephemeris descriptor                         
   @param target (in) id to be converted
*/
/*--------------------------------------------------------------------------*/
static int calceph_inpop_convertid_spiceid2old_id(int naiftarget)
{
    int res = -1;

    switch (naiftarget)
    {
        case NAIFID_SOLAR_SYSTEM_BARYCENTER:
            res = 12;
            break;
        case NAIFID_MERCURY_BARYCENTER:
            res = 1;
            break;
        case NAIFID_VENUS_BARYCENTER:
            res = 2;
            break;
        case NAIFID_EARTH_MOON_BARYCENTER:
            res = 13;
            break;
        case NAIFID_MARS_BARYCENTER:
            res = 4;
            break;
        case NAIFID_JUPITER_BARYCENTER:
            res = 5;
            break;
        case NAIFID_SATURN_BARYCENTER:
            res = 6;
            break;
        case NAIFID_URANUS_BARYCENTER:
            res = 7;
            break;
        case NAIFID_NEPTUNE_BARYCENTER:
            res = 8;
            break;
        case NAIFID_PLUTO_BARYCENTER:
            res = 9;
            break;
        case NAIFID_SUN:
            res = 11;
            break;
        case NAIFID_TIME_CENTER:
            res = 0;
            break;
        case NAIFID_TIME_TTMTDB:
            res = 16;
            break;
        case NAIFID_TIME_TCGMTCB:
            res = 17;
            break;
        case NAIFID_EARTH:
            res = 3;
            break;
        case NAIFID_MOON:
            res = 10;
            break;
        default:
            if (naiftarget > CALCEPH_ASTEROID)
            {
                res = naiftarget;
            }
            break;
    }
    return res;
}

/********************************************************/
/*! Check the value of target, center and unit.
  return the value in the old numbering system
  
   @param target (in) "target" object (NAIF or old numbering system)
   @param center (in) "center" object (NAIF or old numbering system) 
   @param unit (in) sum of CALCEPH_UNIT_??? and /or CALCEPH_USE_NAIFID
   If unit& CALCEPH_USE_NAIFID !=0 then NAIF identification numbers are used
   If unit& CALCEPH_USE_NAIFID ==0 then old identification numbers are used
   @param target_oldid (out) "target" object (old numbering system)
   @param center_oldid (out) "center" object (old numbering system) 
   @param newunit (out) sum of CALCEPH_UNIT_??? 
*/
/********************************************************/
int calceph_inpop_compute_unit_check(int target, int center, int unit,
                                     int *target_oldid, int *center_oldid, int *newunit)
{
    if ((unit & CALCEPH_USE_NAIFID) != 0)
    {
        *target_oldid = calceph_inpop_convertid_spiceid2old_id(target);
        *center_oldid = calceph_inpop_convertid_spiceid2old_id(center);
        if (*target_oldid == -1)
        {
            fatalerror("Target object %d is not available in the ephemeris file.\n", target);
            return 0;
        }
        if (*center_oldid == -1)
        {
            fatalerror("Center object %d is not available in the ephemeris file.\n", center);
            return 0;
        }
        *newunit = unit - CALCEPH_USE_NAIFID;
    }
    else
    {
        *target_oldid = target;
        *center_oldid = center;
        if (*target_oldid > TCGMTCB + 1 && *target_oldid < CALCEPH_ASTEROID)
        {
            fatalerror("Target object %d is not available in the ephemeris file.\n", target);
            return 0;
        }
        if (*center_oldid > TCGMTCB + 1 && *center_oldid < CALCEPH_ASTEROID)
        {
            fatalerror("Center object %d is not available in the ephemeris file.\n", center);
            return 0;
        }
        if (*center_oldid != 0 &&
            (target == NUTATIONS + 1 || target == LIBRATIONS + 1 || target == TTMTDB + 1 || target == TCGMTCB + 1))
        {
            fatalerror("Center object should be 0 (instead of %d) for the given target %d.\n", center, target);
            return 0;
        }
        *newunit = unit;
    }
    return 1;
}

/********************************************************/
/*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
    for a given target and center  at the specified time 
  (time elapsed from JD0).
  The output is expressed according to unit.
  The target and center are expressed in the old or NAIF ID numbering system defined on unit.
 
  return 0 on error.
  return 1 on success.

   @param pephbin (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object (NAIF or old numbering system)
   @param center (in) "center" object (NAIF or old numbering system) 
   @param unit (in) sum of CALCEPH_UNIT_??? and /or CALCEPH_USE_NAIFID
   If unit& CALCEPH_USE_NAIFID !=0 then NAIF identification numbers are used
   If unit& CALCEPH_USE_NAIFID ==0 then old identification numbers are used
   @param order (in) order of the computation
   =0 : Positions are  computed
   =1 : Position+Velocity  are computed
   =2 : Position+Velocity+Acceleration  are computed
   =3 : Position+Velocity+Acceleration+Jerk  are computed
   @param PV (out) quantities of the "target" object
   PV[0:3*(order+1)-1] are valid
*/
/********************************************************/
int calceph_inpop_compute_unit(t_calcephbin * pephbin, double JD0, double time, int target,
                               int center, int unit, int order, double PV[ /*<=12 */ ])
{
    int target_oldid;

    int center_oldid;

    int newunit;

    int res = calceph_inpop_compute_unit_check(target, center, unit, &target_oldid, &center_oldid, &newunit);

    if (res)
    {
        res = calceph_inpop_compute_unit_oldid(pephbin, JD0, time, target_oldid, center_oldid, newunit, order, PV);
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! Return the orientation of the object 
    for a given target at the specified time 
  (time elapsed from JD0).
  The output is expressed according to unit.
  The target  is expressed in the old or NAIF ID numbering system defined on unit.
 
  return 0 on error.
  return 1 on success.

   @param pephbin (inout) ephemeris file descriptor
   @param JD0 (in) reference time (could be 0)
   @param time (in) time elapsed from JD0
   @param target (in) "target" object (NAIF or old numbering system)
   @param unit (in) sum of CALCEPH_UNIT_??? and /or CALCEPH_USE_NAIFID
   If unit& CALCEPH_USE_NAIFID !=0 then NAIF identification numbers are used
   If unit& CALCEPH_USE_NAIFID ==0 then old identification numbers are used
   @param order (in) order of the computation
   =0 : Positions are  computed
   =1 : Position+Velocity  are computed
   =2 : Position+Velocity+Acceleration  are computed
   =3 : Position+Velocity+Acceleration+Jerk  are computed
   @param PV (out) quantities of the "target" object
     PV[0:3*(order+1)-1] are valid
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_orient_unit(t_calcephbin * pephbin, double JD0, double time, int target, int unit, int order,
                              double PV[ /*<=12 */ ])
{
    int target_oldid;

    int newunit;

    int orientmask = (unit & (CALCEPH_OUTPUT_NUTATIONANGLES + CALCEPH_OUTPUT_EULERANGLES));

    /* if no orientation output are specified => euler anges are requested */
    if (orientmask == 0)
    {
        orientmask = CALCEPH_OUTPUT_EULERANGLES;
        unit += CALCEPH_OUTPUT_EULERANGLES;
    }

    if ((unit & CALCEPH_USE_NAIFID) != 0)
    {
        bool bfailure = true;

        if (target == NAIFID_MOON && orientmask == (unit & CALCEPH_OUTPUT_EULERANGLES))
        {
            bfailure = false;
            target_oldid = LIBRATIONS + 1;
        }
        if (target == NAIFID_EARTH && orientmask == (unit & CALCEPH_OUTPUT_NUTATIONANGLES))
        {
            bfailure = false;
            target_oldid = NUTATIONS + 1;
        }
        if (bfailure)
        {
            if ((unit & CALCEPH_OUTPUT_NUTATIONANGLES) != 0)
                fatalerror("Orientation for the target object %d is not supported : Nutation angles.\n", target);
            fatalerror("Orientation for the target object %d is not supported.\n", target);
            return 0;
        }
        newunit = unit - CALCEPH_USE_NAIFID;
    }
    else
    {
        bool bfailure = true;

        if (target == LIBRATIONS + 1 && orientmask == (unit & CALCEPH_OUTPUT_EULERANGLES))
        {
            bfailure = false;
            target_oldid = LIBRATIONS + 1;
        }
        if (target == NUTATIONS + 1 && orientmask == (unit & CALCEPH_OUTPUT_NUTATIONANGLES))
        {
            bfailure = false;
            target_oldid = NUTATIONS + 1;
        }
        if (bfailure)
        {
            if ((unit & CALCEPH_OUTPUT_NUTATIONANGLES) != 0)
                fatalerror("Orientation for the target object %d is not supported : Nutation angles.\n", target);
            fatalerror("Orientation for the target object %d is not supported.\n", target);
            return 0;
        }
        newunit = unit;
    }
    return calceph_inpop_compute_unit_oldid(pephbin, JD0, time, target_oldid, 0, newunit, order, PV);
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function reads the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only on all targets.
     for asteroids, the target value is "asteroid number"+CALCEPH_ASTEROID
     
     It computes position and velocity vectors for a selected  
     planetary body from Chebyshev coefficients.          

                                                                          
  @param pephbin (inout) ephemeris file descriptor                           
       The data of p_pbinfile is not updated if data are prefetched (prefetch!=0) 
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff (in) Time for which position is desired (Julian Date)
  @param Target   (in) Solar system body for which position is desired.         
  @param Center   (in) Solar system body from which position is desired.         
  @param Unit  (in) output unit of the statetype
  @param Planet (out) position and velocity                                                                        
         the units are expressed in Unit units.
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_interpol_PV(struct calcephbin_inpop *pephbin, treal TimeJD0, treal Timediff,
                              int Target, int PARAMETER_UNUSED(Center), int Unit, stateType * Planet)
{
#if HAVE_PRAGMA_UNUSED
#pragma unused(Center)
#endif
    treal Time = TimeJD0 + Timediff;

    int res = 0;

    int ephunit;

    /* check if time is in the ephemeris */
    if (Time < pephbin->H1.timeData[0] || Time > pephbin->H1.timeData[1])
    {
        fatalerror("time %g isn't in the interval [%.16g , %.16g] \n",
                   Time, pephbin->H1.timeData[0], pephbin->H1.timeData[1]);
        return 0;
    }

    if (Target >= CALCEPH_ASTEROID)
    {                           /* add 1 to target because Target is decreased by calceph_compute */
        res =
            calceph_interpol_PV_asteroid(&pephbin->asteroids, TimeJD0, Timediff, Target + 1, &ephunit, pephbin->isinAU,
                                         Planet);
    }
    else
    {
        res = calceph_interpol_PV_planet(pephbin, TimeJD0, Timediff, Target, &ephunit, Planet);
    }
    if (res)
        res = calceph_inpop_unit_convert(pephbin, Planet, Target, ephunit, Unit);
    return res;
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function check the inputs for the function calceph_interpol_PV_planet.
     It computes the value C, G, N, pncomp and ephunit
     
     return 0 if an error occurs
     return 1 if successfull
                                                                          
  @param p_pbinfile (inout) descripteur de l'ephemeride                            
  @param Target   (in) Solar system body for which position is desired.         
  @param ephunit  (out) output unit of the statetype
  @param pC (out)   Coeff array entry point                                       
  @param pN (out)    Number of coeff's                                           
  @param pG (out)   Granules in current record          
  @param pncomp (out) number of components in the file                          
*/
/*--------------------------------------------------------------------------*/
int calceph_interpol_PV_planet_check(const struct calcephbin_inpop *p_pbinfile,
                                     int Target, int *ephunit, int *pC, int *pG, int *pN, int *pncomp)
{
    int C, G, N;

    /*--------------------------------------------------------------------------*/
    /* Read the coefficients from the binary record.                            */
    /*--------------------------------------------------------------------------*/
    *pncomp = p_pbinfile->coefftime_array.ncomp;

    if (Target == NUTATIONS)
    {
        C = p_pbinfile->H1.coeffPtr[11][0] - 1;
        N = p_pbinfile->H1.coeffPtr[11][1];
        G = p_pbinfile->H1.coeffPtr[11][2];
        *ephunit = CALCEPH_UNIT_RAD + (p_pbinfile->isinDay ? CALCEPH_UNIT_DAY : CALCEPH_UNIT_SEC);
        *pncomp = 2;
        if (!p_pbinfile->haveNutation)
        {
            fatalerror(" The file doesn't have IAU 1980 nutations angles.\n");
            return 0;
        }
    }
    else if (Target == LIBRATIONS)
    {                           /*Librations */
        C = p_pbinfile->H1.libratPtr[0] - 1;
        N = p_pbinfile->H1.libratPtr[1];
        G = p_pbinfile->H1.libratPtr[2];
        *ephunit = CALCEPH_UNIT_RAD + (p_pbinfile->isinDay ? CALCEPH_UNIT_DAY : CALCEPH_UNIT_SEC);
    }
    else if (Target == TTMTDB)
    {                           /*TT-TDB */
#if DEBUG
        printf("target=TTMTDB\n");
#endif
        if (!p_pbinfile->haveTTmTDB)
        {
            fatalerror(" The file doesn't have TT-TDB data.\n");
            return 0;
        }
        if (p_pbinfile->timescale != etimescale_TDB)
        {
            fatalerror(" The file is not expressed in the TDB time scale.\n");
            return 0;
        }
        C = p_pbinfile->H1.TTmTDBPtr[0] - 1;
        N = p_pbinfile->H1.TTmTDBPtr[1];
        G = p_pbinfile->H1.TTmTDBPtr[2];
        *pncomp = p_pbinfile->coefftime_array.ncompTTmTDB;
        *ephunit = CALCEPH_UNIT_SEC;
    }
    else if (Target == TCGMTCB)
    {                           /*TCG-TCB */
#if DEBUG
        printf("target=TCGMTCB p_pbinfile->timescale=%d\n", (int) p_pbinfile->timescale);
#endif
        if (!p_pbinfile->haveTTmTDB)
        {
            fatalerror(" The file doesn't have TCG-TCB data.\n");
            return 0;
        }
        if (p_pbinfile->timescale != etimescale_TCB)
        {
            fatalerror(" The file is not expressed in the TCB time scale.\n");
            return 0;
        }
        C = p_pbinfile->H1.TTmTDBPtr[0] - 1;
        N = p_pbinfile->H1.TTmTDBPtr[1];
        G = p_pbinfile->H1.TTmTDBPtr[2];
        *ephunit = CALCEPH_UNIT_SEC;
    }
    else
    {                           /* Planets */
        C = p_pbinfile->H1.coeffPtr[Target][0] - 1;
        N = p_pbinfile->H1.coeffPtr[Target][1];
        G = p_pbinfile->H1.coeffPtr[Target][2];
        *ephunit = (p_pbinfile->isinAU ? CALCEPH_UNIT_AU : CALCEPH_UNIT_KM)
            + (p_pbinfile->isinDay ? CALCEPH_UNIT_DAY : CALCEPH_UNIT_SEC);
    }
    *pC = C;
    *pG = G;
    *pN = N;

    return 1;
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function reads the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only on planets/libration/moon/TT-TDB.
     
     It computes position and velocity vectors for a selected  
     planetary body from Chebyshev coefficients.          

                                                                          
  @param p_pbinfile (inout) ephemerids descriptor 
       The data of p_pbinfile is not updated if data are prefetched (prefetch!=0) 
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff (in) Time for which position is desired (Julian Date)
  @param Target   (in) Solar system body for which position is desired.         
  @param Position (in) Pointer to external array to receive the position.       
  @param ephunit  (out) output unit of the statetype
  @param Planet (out) position and velocity                                                                        
         the units are expressed in ephunit units.
*/
/*--------------------------------------------------------------------------*/
static int calceph_interpol_PV_planet(struct calcephbin_inpop *p_pbinfile, treal TimeJD0, treal Timediff,
                                      int Target, int *ephunit, stateType * Planet)
{
    treal Time = TimeJD0 + Timediff;

    int C, G, N, ncomp;

    int res;

    t_memarcoeff localmemarrcoeff;  /* local data structure for thread-safe */

    /*--------------------------------------------------------------------------*/
    /* Determine if a new record needs to be input.                             */
    /*--------------------------------------------------------------------------*/

    localmemarrcoeff = p_pbinfile->coefftime_array;
    if (Time < p_pbinfile->coefftime_array.T_beg || Time > p_pbinfile->coefftime_array.T_end)
    {
        if (!calceph_inpop_seekreadcoeff(&localmemarrcoeff, Time))
            return 0;
    }

    res = calceph_interpol_PV_planet_check(p_pbinfile, Target, ephunit, &C, &G, &N, &ncomp);
    if (res)
    {
        calceph_interpol_PV_an(&localmemarrcoeff, TimeJD0, Timediff, Planet, C, G, N, ncomp);
    }

    /* update the global data structure only if no prefetch */
    if (p_pbinfile->coefftime_array.prefetch == 0)
    {
        p_pbinfile->coefftime_array = localmemarrcoeff;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors                 
     from Chebyshev coefficients.                                    
                                          
                                                                          
 @param Tc  (in) normalized time for Chebyshev
 @param X (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units                                 
   Planet has velocity in ephemerides units  
   ...                               
 @param N (in)    Number of coeff's                                           
 @param ncomp (in) number of components                                  
 @param scale (in)   scale to apply on the derivates                                   
 @param A (in) coefficient array                                  
*/
/*--------------------------------------------------------------------------*/
void calceph_interpol_PV_lowlevel(stateType * X, const treal * A, treal Tc, treal scale, int N, int ncomp)
{
#if __STDC_VERSION__ > 199901L
#define NSTACK_INTERPOL (N + 1)
#else
#define NSTACK_INTERPOL 15000
#endif
    treal Cp[NSTACK_INTERPOL], Up[NSTACK_INTERPOL], Vp[NSTACK_INTERPOL], Wp[NSTACK_INTERPOL];

#undef NSTACK_INTERPOL

    /* Compute interpolated position */
    calceph_chebyshev_order_0(Cp, N, Tc);
    calceph_interpolate_chebyshev_order_0_stride_0(ncomp, X->Position, N, Cp, A);

    if (ncomp <= 3)
    {
        /* Compute interpolated velocity */
        if (X->order >= 1)
        {
            calceph_chebyshev_order_1(Up, N, Tc, Cp);
            calceph_interpolate_chebyshev_order_1_stride_0(ncomp, X->Velocity, N, Up, A, scale);
        }

        /* Compute interpolated acceleration */
        if (X->order >= 2)
        {
            calceph_chebyshev_order_2(Vp, N, Tc, Up);
            calceph_interpolate_chebyshev_order_2_stride_0(ncomp, X->Acceleration, N, Vp, A, scale * scale);
        }

        /* Compute interpolated jerk */
        if (X->order >= 3)
        {
            calceph_chebyshev_order_3(Wp, N, Tc, Vp);
            calceph_interpolate_chebyshev_order_3_stride_0(ncomp, X->Jerk, N, Wp, A, scale * scale * scale);
        }
    }
    else
    {
        /* Compute interpolated velocity */
        if (X->order >= 1)
        {
            calceph_interpolate_chebyshev_order_0_stride_3(X->Velocity, N, Cp, A);
        }

        /* Compute interpolated acceleration */
        if (X->order >= 2)
        {
            calceph_chebyshev_order_1(Up, N, Tc, Cp);
            calceph_interpolate_chebyshev_order_1_stride_3(X->Acceleration, N, Up, A, scale);
        }

        /* Compute interpolated jerk */
        if (X->order >= 3)
        {
            calceph_chebyshev_order_2(Vp, N, Tc, Up);
            calceph_interpolate_chebyshev_order_2_stride_3(X->Jerk, N, Vp, A, scale * scale);
        }
    }
}

/*--------------------------------------------------------------------------*/
/*!  This function computes position and velocity vectors                 
     from Chebyshev coefficients These coefficients are stored at offset  
     C, G in  the object p_pbinfile.                                      
                                          
                                                                          
 @param p_pbinfile (inout) ephemeris descriptor                            
 @param TimeJD0  (in) Time for which position is desired (Julian Date)
 @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
 @param Planet (out) Pointer to external array to receive the position/velocity.
   Planet has position in ephemerides units                                 
   Planet has velocity in ephemerides units                                 
 @param C (in)   Coeff array entry point                                       
 @param N (in)    Number of coeff's                                           
 @param G (in)   Granules in current record  
 @param ncomp (in) number of components                                  
*/
/*--------------------------------------------------------------------------*/
void calceph_interpol_PV_an(const t_memarcoeff * p_pbinfile, treal TimeJD0,
                            treal Timediff, stateType * Planet, int C, int G, int N, int ncomp)
{
    treal T_break, T_sub, Tc, scale;

    treal Time = TimeJD0 + Timediff;

    long long int TimeJD0pent = longr(TimeJD0);

    treal TimeJD0frac = TimeJD0 - realr(TimeJD0pent);

    long long int Timediffpent = longr(Timediff);

    treal Timedifffrac = Timediff - realr(Timediffpent);

    long long int T_begpent;

    treal T_begfrac;

    treal deltapent, deltafrac;

    int j;

    long int offset = 0;

    long int lC = C;

    stateType X;

    const treal *A;

    X.order = Planet->order;
    /*--------------------------------------------------------------------------*/
    /* Initialize local coefficient array.                                      */
    /*--------------------------------------------------------------------------*/
    T_begpent = longr(p_pbinfile->T_beg);
    T_begfrac = p_pbinfile->T_beg - T_begpent;
    if (G == 1)
    {
        /* code equivalent a :
           Tc = 2.0E0*(Time - T_beg) / T_span - 1.0E0; */
        deltapent = (TimeJD0pent + Timediffpent - T_begpent) / p_pbinfile->T_span;
        deltafrac = (TimeJD0frac + Timedifffrac - T_begfrac) / p_pbinfile->T_span;
    }
    else
    {
        T_sub = p_pbinfile->T_span / realr(G);  /* Compute subgranule interval */

        for (j = G; j > 0; j--)
        {
            T_break = p_pbinfile->T_beg + realr(j - 1) * T_sub;
            if (Time > T_break)
            {
                /* T_seg  = T_break; */
                offset = j - 1;
                break;
            }
        }
        /* code equivalent a :
           Tc = 2.0E0*(Time - T_seg) / T_sub - 1.0E0; */
        deltapent = (TimeJD0pent + Timediffpent - T_begpent) / T_sub;
        deltafrac = (TimeJD0frac + Timedifffrac - T_begfrac) / T_sub - realr(offset);

        lC = lC + ncomp * offset * N;
    }
    Tc = REALC(2.0E0) * (deltapent + deltafrac) - REALC(1.0E0);
    A = p_pbinfile->Coeff_Array + lC;

    /*--------------------------------------------------------------------------*/
    /* Compute the interpolated position and velocity                           */
    /*--------------------------------------------------------------------------*/
    scale = REALC(2.0E0) * realr(G) / (p_pbinfile->T_span);

    calceph_interpol_PV_lowlevel(&X, A, Tc, scale, N, ncomp);

    /*--------------------------------------------------------------------------*/
    /*  Return computed values.                                                 */
    /*--------------------------------------------------------------------------*/

    *Planet = X;
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function changes the state vector, which is expressed in InputUnit units as input,  
       to the OutputUnit as output.
     
     This function is for INPOP ephemeris format.

  it returns 0 if the function fails to convert.
                                                                          
  @param pephbin (inout) ephemeris descriptor                         
  @param Target   (in) Solar system body for which position is desired.         
  @param InputUnit  (in) output unit of the statetype
  @param OutputUnit  (in) output unit of the statetype
  @param Planet (inout) position and velocity                                                                        
          Input : expressed in InputUnit units.
          Output : expressed in OutputUnit units.
*/
/*--------------------------------------------------------------------------*/
static int calceph_inpop_unit_convert(const struct calcephbin_inpop *pephbin, stateType * Planet,
                                      int Target, int InputUnit, int OutputUnit)
{
    int res = 1;

    /* if same unit in input and output => do nothing */
    if (InputUnit == OutputUnit)
        return res;

    /* some checks */
    switch (Target)
    {
        case NUTATIONS:
        case LIBRATIONS:       /*Librations */
            if ((OutputUnit & CALCEPH_UNIT_RAD) == 0)
            {
                res = 0;
                fatalerror("Units for libration or nutations must be in radians\n");
            }
            if ((InputUnit & CALCEPH_UNIT_RAD) == 0)
            {
                res = 0;
                fatalerror("Input units for libration or nutations must be in radians\n");
            }
            if (res)
                res = calceph_unit_convert_quantities_time(Planet, InputUnit, OutputUnit);
            break;

        case TCGMTCB:          /*TCG-TCB */
        case TTMTDB:           /*TT-TDB */
            if ((OutputUnit & CALCEPH_UNIT_DAY) == CALCEPH_UNIT_DAY
                && (InputUnit & CALCEPH_UNIT_SEC) == CALCEPH_UNIT_SEC)
            {
                Planet->Position[0] /= 86400E0;
            }
            if ((InputUnit & CALCEPH_UNIT_DAY) == CALCEPH_UNIT_DAY
                && (OutputUnit & CALCEPH_UNIT_SEC) == CALCEPH_UNIT_SEC)
            {
                Planet->Position[0] *= 86400E0;
            }
            if ((OutputUnit & (CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)) == 0)
            {
                res = 0;
                fatalerror("Units must include CALCEPH_UNIT_DAY or CALCEPH_UNIT_SEC \n");
            }
            if ((OutputUnit & (CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)) == CALCEPH_UNIT_DAY + CALCEPH_UNIT_SEC)
            {
                res = 0;
                fatalerror("Units must include only one value : CALCEPH_UNIT_DAY or CALCEPH_UNIT_SEC \n");
            }
            break;

        default:
            if ((OutputUnit & (CALCEPH_UNIT_AU + CALCEPH_UNIT_KM)) == 0)
            {
                res = 0;
                fatalerror("Units must include CALCEPH_UNIT_AU or CALCEPH_UNIT_KM \n");
            }
            if ((OutputUnit & (CALCEPH_UNIT_AU + CALCEPH_UNIT_KM)) == CALCEPH_UNIT_AU + CALCEPH_UNIT_KM)
            {
                res = 0;
                fatalerror("Units must include only one value : CALCEPH_UNIT_AU or CALCEPH_UNIT_KM \n");
            }

            if ((OutputUnit & CALCEPH_UNIT_KM) == CALCEPH_UNIT_KM && (InputUnit & CALCEPH_UNIT_AU) == CALCEPH_UNIT_AU)
            {
                treal rau = pephbin->H1.AU;

                calceph_stateType_mul_scale(Planet, rau);
            }
            if ((InputUnit & CALCEPH_UNIT_KM) == CALCEPH_UNIT_KM && (OutputUnit & CALCEPH_UNIT_AU) == CALCEPH_UNIT_AU)
            {
                treal rau = pephbin->H1.AU;

                calceph_stateType_div_scale(Planet, rau);
            }
            if (res)
                res = calceph_unit_convert_quantities_time(Planet, InputUnit, OutputUnit);
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return EMRAT from file 
  @param p_pbinfile (in) ephemeris file
*/
/*--------------------------------------------------------------------------*/
double calceph_inpop_getEMRAT(struct calcephbin_inpop *p_pbinfile)
{
    return p_pbinfile->H1.EMRAT;
}

/*--------------------------------------------------------------------------*/
/*! return AU from file 
  @param p_pbinfile (in) ephemeris file
*/
/*--------------------------------------------------------------------------*/
double calceph_inpop_getAU(struct calcephbin_inpop *p_pbinfile)
{
    return p_pbinfile->H1.AU;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from file 
  return 0 if constant is not found
  return 1 if constant is found and set p_pdval.
  @param p_pbinfile (in) ephemeris file
  @param name (in) constant name
  @param p_pdval (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_getconstant(struct calcephbin_inpop *p_pbinfile, const char *name, double *p_pdval)
{
    int found = 0;

    int cpt = 0;

    size_t len = strlen(name);

    if (len <= 6)
    {
        while (cpt < p_pbinfile->H1.numConst && found == 0)
        {
            if (strncmp(name, (const char *) (p_pbinfile->H1.constName[cpt]), len) == 0)
            {
                size_t k;

                for (k = len; k < 6; k++)
                {
                    if (p_pbinfile->H1.constName[cpt][k] != ' ' && p_pbinfile->H1.constName[cpt][k] != '\0')
                        break;
                }
                if (k == 6)
                {
                    found = 1;
                    *p_pdval = p_pbinfile->constVal[cpt];
                }
            }
            cpt++;
        }
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! return the number of constants available in the ephemeris file 
  
  @param eph (inout) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_getconstantcount(struct calcephbin_inpop *eph)
{
    return eph->H1.numConst + 2;    /*+2 <=> AU + EMRAT */
}

/*--------------------------------------------------------------------------*/
/*! get the name and its value of the constant available 
    at some index from the ephemeris file 
   return 1 if index is valid
   return 0 if index isn't valid
  
  @param eph (inout) ephemeris descriptor
  @param index (in) index of the constant (must be between 1 and calceph_getconstantcount() )
  @param name (out) name of the constant 
  @param value (out) value of the constant 
*/
/*--------------------------------------------------------------------------*/
int calceph_inpop_getconstantindex(struct calcephbin_inpop *eph, int index, char name[CALCEPH_MAX_CONSTANTNAME],
                                   double *value)
{
    int res;

    int j;

    int nconst = calceph_inpop_getconstantcount(eph);

    if (index >= 1 && index <= nconst)
    {
        res = 1;
        for (j = 0; j < CALCEPH_MAX_CONSTANTNAME; j++)
            name[j] = ' ';      /* initialization for fortran */
        if (index == nconst)
        {
            *value = calceph_inpop_getAU(eph);
            strcpy(name, "AU");
        }
        else if (index == nconst - 1)
        {
            *value = calceph_inpop_getEMRAT(eph);
            strcpy(name, "EMRAT");
        }
        else
        {
            strncpy(name, eph->H1.constName[index - 1], 6 /*CALCEPH_MAX_CONSTANTNAME-1 */ );
            *value = eph->constVal[index - 1];
        }
        name[6 /*CALCEPH_MAX_CONSTANTNAME-1 */ ] = '\0';
    }
    else
    {
        res = 0;
    }
    return res;
}
