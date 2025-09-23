/*-----------------------------------------------------------------*/
/*! 
  \file calcephast.c 
  \brief perform I/O and several computations for asteroids.
         for inpop ephemerides only.

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2011-2024, CNRS
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
#ifndef __cplusplus
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#endif /*__cplusplus */
#if HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinpop.h"
#include "util.h"

#if DEBUG
static void calceph_debug_asteroid(const t_ast_calcephbin * infoast);
#endif

/*--------------------------------------------------------------------------*/
/*!  Initialize to "NULL" all fields of p_pasteph.     
  
  @param p_pasteph (out) asteroids ephemeris.   
*/
/*--------------------------------------------------------------------------*/
void calceph_empty_asteroid(t_ast_calcephbin * p_pasteph)
{
    p_pasteph->inforec.locNextRecord = 0;
    p_pasteph->inforec.numAstRecord = 0;
    p_pasteph->inforec.numAst = 0;
    p_pasteph->coefftime_array.file = NULL;
    p_pasteph->coefftime_array.Coeff_Array = NULL;
    p_pasteph->coefftime_array.mmap_buffer = NULL;
    p_pasteph->coefftime_array.mmap_size = 0;
    p_pasteph->coefftime_array.mmap_used = 0;
    p_pasteph->coefftime_array.prefetch = 0;
    p_pasteph->id_array = NULL;
    p_pasteph->GM_array = NULL;
    p_pasteph->coeffptr_array = NULL;
}

/*--------------------------------------------------------------------------*/
/*!  free memory used by  p_pasteph.     
  
  @param p_pasteph (inout) asteroids ephemeris.   
*/
/*--------------------------------------------------------------------------*/
void calceph_free_asteroid(t_ast_calcephbin * p_pasteph)
{
    if (p_pasteph->coefftime_array.prefetch == 0)
    {
        if (p_pasteph->coefftime_array.Coeff_Array != NULL)
            free(p_pasteph->coefftime_array.Coeff_Array);
    }
    if (p_pasteph->id_array != NULL)
        free(p_pasteph->id_array);
    if (p_pasteph->GM_array != NULL)
        free(p_pasteph->GM_array);
    if (p_pasteph->coeffptr_array != NULL)
        free(p_pasteph->coeffptr_array);
    /* the prefetch fields are not freed because there are shared with the planetary records */
    calceph_empty_asteroid(p_pasteph);
}

/*--------------------------------------------------------------------------*/
/*!  Initialize  p_pasteph.   
     Read records from the file.
  @return returns 1 on success. return 0 on failure.
  
  @param p_pasteph (inout) asteroids ephemeris. 
  @param file (inout) binary ephemeris file
  @param swapbyte (in) =0 => don't swap byte order of the data
  @param timeData (in) begin/end/step of the time in the file
  @param recordsize (in) size of a record in number of coefficients
  @param ncomp (in) number of component in the binary ephemeris file
  @param locnextrecord (inout) on enter, location to read the Information asteroid record
            on exit, the location of the next record (could be 0 if no other record)
*/
/*--------------------------------------------------------------------------*/
int calceph_init_asteroid(t_ast_calcephbin * p_pasteph, FILE * file, int swapbyte, double timeData[3], int recordsize,
                          int ncomp, int *locnextrecord)
{
    size_t recordsizebytes = sizeof(double) * recordsize;
    off_t offset;
    buffer_error_t buffer_error;

    p_pasteph->coefftime_array.file = file;
    p_pasteph->coefftime_array.swapbyteorder = swapbyte;
    p_pasteph->coefftime_array.ncomp = ncomp;

   /*--------------------------------------------------------------------------*/
    /* read the Information asteroid record */
   /*--------------------------------------------------------------------------*/
    offset = (*locnextrecord) - 1;

#if DEBUG
    printf("read Information asteroid record at slot %lu\n", (unsigned long) offset);
#endif
    if (fseeko(file, offset * recordsizebytes, SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to Information Asteroid record \nSystem error : '%s'\n",
                   calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    if (fread(&p_pasteph->inforec, sizeof(t_InfoAstRecord), 1, file) != 1)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read Information Asteroid record\n");
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    /*swap */
    if (swapbyte)
    {
        p_pasteph->inforec.locNextRecord = swapint(p_pasteph->inforec.locNextRecord);
        p_pasteph->inforec.numAst = swapint(p_pasteph->inforec.numAst);
        p_pasteph->inforec.numAstRecord = swapint(p_pasteph->inforec.numAstRecord);
        p_pasteph->inforec.typeAstRecord = swapint(p_pasteph->inforec.typeAstRecord);
        p_pasteph->inforec.locIDAstRecord = swapint(p_pasteph->inforec.locIDAstRecord);
        p_pasteph->inforec.numIDAstRecord = swapint(p_pasteph->inforec.numIDAstRecord);
        p_pasteph->inforec.locGMAstRecord = swapint(p_pasteph->inforec.locGMAstRecord);
        p_pasteph->inforec.numGMAstRecord = swapint(p_pasteph->inforec.numGMAstRecord);
        p_pasteph->inforec.locCoeffPtrAstRecord = swapint(p_pasteph->inforec.locCoeffPtrAstRecord);
        p_pasteph->inforec.numCoeffPtrAstRecord = swapint(p_pasteph->inforec.numCoeffPtrAstRecord);
        p_pasteph->inforec.locCoeffAstRecord = swapint(p_pasteph->inforec.locCoeffAstRecord);
        p_pasteph->inforec.numCoeffAstRecord = swapint(p_pasteph->inforec.numCoeffAstRecord);
    }

   /*--------------------------------------------------------------------------*/
    /* check supported asteroid type by the library  */
   /*--------------------------------------------------------------------------*/
    if (p_pasteph->inforec.typeAstRecord != 1)
    {
        /* GCOVR_EXCL_START */
        fatalerror("CALCEPH library %d.%d.%d does not support this type of asteroid : %d\n",
                   CALCEPH_VERSION_MAJOR, CALCEPH_VERSION_MINOR, CALCEPH_VERSION_PATCH,
                   p_pasteph->inforec.typeAstRecord);
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    if (p_pasteph->inforec.numAst > 0)
    {
   /*--------------------------------------------------------------------------*/
        /* allocate memory for data  */
   /*--------------------------------------------------------------------------*/
        p_pasteph->id_array = (int *) malloc(sizeof(int) * p_pasteph->inforec.numAst);
        if (p_pasteph->id_array == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for %d integers\nSystem error : '%s'\n",
                       p_pasteph->inforec.numAst, calceph_strerror_errno(buffer_error));
            return 0;
            /* GCOVR_EXCL_STOP */
        }

        p_pasteph->GM_array = (double *) malloc(sizeof(double) * p_pasteph->inforec.numAst);
        if (p_pasteph->GM_array == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for %d reals\nSystem error : '%s'\n",
                       p_pasteph->inforec.numAst, calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }

        p_pasteph->coeffptr_array = (t_CoeffPtr *) malloc(sizeof(t_CoeffPtr) * p_pasteph->inforec.numAst);
        if (p_pasteph->coeffptr_array == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for %d integers\nSystem error : '%s'\n",
                       p_pasteph->inforec.numAst * 3, calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }

    /*--------------------------------------------------------------------------*/
        /* read ID Asteroid record */
    /*--------------------------------------------------------------------------*/
#if DEBUG
        printf("read ID asteroid record at slot %d\n", p_pasteph->inforec.locIDAstRecord - 1);
#endif
        if (fseeko(file, ((off_t) (p_pasteph->inforec.locIDAstRecord - 1)) * ((off_t) recordsizebytes), SEEK_SET) != 0)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't jump to the ID Asteroid record \nSystem error : '%s'\n",
                       calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        if (fread(p_pasteph->id_array, sizeof(int), p_pasteph->inforec.numAst, file) !=
            (size_t) p_pasteph->inforec.numAst)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't read  the ID Asteroid record\n");
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }

    /*--------------------------------------------------------------------------*/
        /* read GM Asteroid record */
    /*--------------------------------------------------------------------------*/
#if DEBUG
        printf("read GM asteroid record at slot %d\n", p_pasteph->inforec.locGMAstRecord - 1);
#endif
        if (fseeko(file, (p_pasteph->inforec.locGMAstRecord - 1) * recordsizebytes, SEEK_SET) != 0)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't jump to  the GM Asteroid record \nSystem error : '%s'\n",
                       calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        if (fread(p_pasteph->GM_array, sizeof(double), p_pasteph->inforec.numAst, file) !=
            (size_t) p_pasteph->inforec.numAst)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't read  the GM Asteroid record\n");
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }

    /*--------------------------------------------------------------------------*/
        /* read Coefficient Pointer Asteroid record */
    /*--------------------------------------------------------------------------*/
#if DEBUG
        printf("read Coefficient Pointer asteroid record at slot %d\n", p_pasteph->inforec.locCoeffPtrAstRecord - 1);
#endif
        if (fseeko(file, (p_pasteph->inforec.locCoeffPtrAstRecord - 1) * recordsizebytes, SEEK_SET) != 0)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't jump to the Coefficient Pointer Asteroid record \nSystem error : '%s'\n",
                       calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        if (fread(p_pasteph->coeffptr_array, sizeof(t_CoeffPtr), p_pasteph->inforec.numAst, file) !=
            (size_t) p_pasteph->inforec.numAst)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't read the Coefficient Pointer Asteroid record\n");
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }

#if DEBUG
        calceph_debug_asteroid(p_pasteph);
#endif

        /*swap */
        if (swapbyte)
        {
            swapintarray(p_pasteph->id_array, p_pasteph->inforec.numAst);
            swapdblarray(p_pasteph->GM_array, p_pasteph->inforec.numAst);
            swapintarray((int *) (p_pasteph->coeffptr_array), p_pasteph->inforec.numAst * 3);
        }

#if DEBUG
        calceph_debug_asteroid(p_pasteph);
#endif

    /*--------------------------------------------------------------------------*/
        /* allocate memory for coefficient record */
    /*--------------------------------------------------------------------------*/
        p_pasteph->coefftime_array.ARRAY_SIZE = recordsize * p_pasteph->inforec.numCoeffAstRecord;
        p_pasteph->coefftime_array.Coeff_Array =
            (double *) malloc(sizeof(double) * p_pasteph->coefftime_array.ARRAY_SIZE);
        if (p_pasteph->coefftime_array.Coeff_Array == NULL)
        {
            fatalerror("Can't allocate memory for %d reals\nSystem error : '%s'\n",
                       p_pasteph->coefftime_array.ARRAY_SIZE, calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;

        }
    /*--------------------------------------------------------------------------*/
        /* read the first coefficient asteroid record */
    /*--------------------------------------------------------------------------*/
        p_pasteph->coefftime_array.offfile = (p_pasteph->inforec.locCoeffAstRecord - 1) * recordsizebytes;
        if (fseeko(file, p_pasteph->coefftime_array.offfile, SEEK_SET) != 0)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't jump to the Coefficient  Asteroid record \nSystem error : '%s'\n",
                       calceph_strerror_errno(buffer_error));
            calceph_free_asteroid(p_pasteph);
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        if (!calceph_inpop_readcoeff(&p_pasteph->coefftime_array, timeData[0]))
        {
            calceph_free_asteroid(p_pasteph);
            return 0;
        }
    }

    *locnextrecord = p_pasteph->inforec.locNextRecord;
    return 1;
}

#if DEBUG
/*--------------------------------------------------------------------------*/
/*!  debug the structure t_ast_calcephbin.
  
  @param infoast (in) asteroids ephemeris. 
*/
/*--------------------------------------------------------------------------*/
static void calceph_debug_asteroid(const t_ast_calcephbin * infoast)
{
    int j;

    printf("locNextRecord: %d\n", infoast->inforec.locNextRecord);
    printf("numAst:%d\n", infoast->inforec.numAst);
    printf("numAstRecord:%d\n", infoast->inforec.numAstRecord);
    printf("typeAstRecord:%d\n", infoast->inforec.typeAstRecord);
    printf("locIDAstRecord:%d\n", infoast->inforec.locIDAstRecord);
    printf("numIDAstRecord:%d\n", infoast->inforec.numIDAstRecord);
    printf("locGMAstRecord:%d\n", infoast->inforec.locGMAstRecord);
    printf("numGMAstRecord:%d\n", infoast->inforec.numGMAstRecord);
    printf("locCoeffPtrAstRecord:%d\n", infoast->inforec.locCoeffPtrAstRecord);
    printf("numCoeffPtrAstRecord:%d\n", infoast->inforec.numCoeffPtrAstRecord);
    printf("locCoeffAstRecord:%d\n", infoast->inforec.locCoeffAstRecord);
    printf("numCoeffAstRecord:%d\n", infoast->inforec.numCoeffAstRecord);

    for (j = 0; j < infoast->inforec.numAst; j++)
    {
        printf("asteroid id=%d GM=%g location=%d ncoeff=%d granule=%d\n",
               infoast->id_array[j], infoast->GM_array[j], infoast->coeffptr_array[j][0],
               infoast->coeffptr_array[j][1], infoast->coeffptr_array[j][2]);
    }

}
#endif /*DEBUG*/
/*--------------------------------------------------------------------------*/
/*!                                                      
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only for asteroids.
     
     It computes position and velocity vectors for a selected  
     planetary body from Chebyshev coefficients.          

                                                                          
  @param p_pbinfile (inout) ephemeris descriptor for asteroids                           
       The data of p_pbinfile is not updated if data are prefetched (prefetch!=0) 
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Target   (in) Solar system body for which position is desired.         
  @param Planet (out) position and velocity                                                                        
         Planet has position in ua                                                
         Planet has velocity in ua/day
  @param isinAU (in) = 1 if unit in the file are in AU       
  @param ephunit (out) units of the output state
*/
/*--------------------------------------------------------------------------*/
int calceph_interpol_PV_asteroid(t_ast_calcephbin * p_pbinfile, treal TimeJD0, treal Timediff,
                                 int Target, int *ephunit, int isinAU, stateType * Planet)
{
    treal Time = TimeJD0 + Timediff;
    int i, id;
    int C, G, N, ncomp;
    t_memarcoeff localmemarrcoeff;  /* local data structure for thread-safe */

    Target = Target - CALCEPH_ASTEROID;

  /*--------------------------------------------------------------------------*/
    /* check if the file contains asteroids.                             */
  /*--------------------------------------------------------------------------*/
    if (p_pbinfile->coefftime_array.file == NULL)
    {
        fatalerror("The ephemeris file doesn't contain any asteroid\n");
        return 0;
    }

  /*--------------------------------------------------------------------------*/
    /* Determine if a new record needs to be input.                             */
  /*--------------------------------------------------------------------------*/
    localmemarrcoeff = p_pbinfile->coefftime_array;

    if (Time < p_pbinfile->coefftime_array.T_beg || Time > p_pbinfile->coefftime_array.T_end)
    {
        if (!calceph_inpop_seekreadcoeff(&localmemarrcoeff, Time))
            return 0;
    }

  /*--------------------------------------------------------------------------*/
    /* Find the asteroid.                                                       */
  /*--------------------------------------------------------------------------*/
    id = -1;
    for (i = 0; i < p_pbinfile->inforec.numAst; i++)
    {
        if (p_pbinfile->id_array[i] == Target)
        {
            id = i;
            break;
        }
    }
    if (id == -1)
    {
        fatalerror("Can't find asteroid %d in the ephemeris file\n", Target);
        return 0;
    }

  /*--------------------------------------------------------------------------*/
    /* Read the coefficients from the binary record.                            */
  /*--------------------------------------------------------------------------*/

    C = p_pbinfile->coeffptr_array[id][0] - 1;
    N = p_pbinfile->coeffptr_array[id][1];
    G = p_pbinfile->coeffptr_array[id][2];
    ncomp = localmemarrcoeff.ncomp;

    calceph_interpol_PV_an(&localmemarrcoeff, TimeJD0, Timediff, Planet, C, G, N, ncomp);
    *ephunit = (isinAU ? CALCEPH_UNIT_AU : CALCEPH_UNIT_KM) + CALCEPH_UNIT_DAY;

    /* update the global data structure only if no prefetch */
    if (p_pbinfile->coefftime_array.prefetch == 0)
    {
        p_pbinfile->coefftime_array = localmemarrcoeff;
    }
    return 1;
}
