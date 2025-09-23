/*-----------------------------------------------------------------*/
/*! 
  \file calcephtxtmkio.c 
  \brief perform I/O on text Meta  KERNEL (MK) ephemeris data file
         load all files from a text Meta KERNEL (MK)  Ephemeris data.
         
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
#endif                          /* __cplusplus */
#include <ctype.h>

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephspice.h"
#include "calcephinternal.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
static int calceph_txtmk_symbols_getlen(const char *buffer, const struct TXTPCKvalue *kernel,
                                        const struct TXTPCKconstant *pathsymbols,
                                        const struct TXTPCKconstant *pathvalues, off_t * slen);
static int calceph_txtmk_symbol_cmpeq(const char *buffer, const struct TXTPCKvalue *symb, off_t loc);
static void calceph_txtmk_symbols_copy(char *dst, const char *buffer, const struct TXTPCKvalue *kernel,
                                       const struct TXTPCKconstant *pathsymbols,
                                       const struct TXTPCKconstant *pathvalues, size_t *curdst);

/*--------------------------------------------------------------------------*/
/*!  Read the files of the text Meta kernel (MK) file
 
   @return  1 on sucess and 0 on failure
 
   @param file (inout) file descriptor.   
   @param filename (in) A character string giving the name of an ephemeris data file.   
   @param res (out) descriptor of the ephemeris.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtmk_open(FILE * file, const char *filename, struct calcephbin_spice *res)
{
    int ret;
    struct TXTPCKfile txtfile;
    buffer_error_t buffer_error;

    ret = calceph_txtpck_load(file, filename, "KPL/MK", res->clocale, &txtfile);

    if (ret)
    {
        struct TXTPCKconstant *kerneltoload;
        struct TXTPCKconstant *pathsymbols;
        struct TXTPCKconstant *pathvalues;

#if DEBUG
        calceph_txtpck_debug(&txtfile);
#endif

        kerneltoload = calceph_txtpck_getptrconstant(&txtfile, "KERNELS_TO_LOAD");
        pathsymbols = calceph_txtpck_getptrconstant(&txtfile, "PATH_SYMBOLS");
        pathvalues = calceph_txtpck_getptrconstant(&txtfile, "PATH_VALUES");

        if (kerneltoload != NULL)
        {
            int nfile = 0;
            char **arfilename;
            struct TXTPCKvalue *kerntmp;

            /* for each value => load the file */
            /* count the number of files */
            for (kerntmp = kerneltoload->value; kerntmp != NULL; kerntmp = kerntmp->next)
            {

                nfile++;        /* except if last character is + */
                if (kerntmp->locfirst < kerntmp->loclast && txtfile.buffer[kerntmp->loclast - 1] == '+')
                {
                    nfile--;
                }
            }

#if DEBUG
            printf("number of files=%d\n", nfile);
#endif
            /* generate the list of file */
            arfilename = (char **) malloc(sizeof(char *) * nfile);
            if (arfilename == NULL)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                           (unsigned long) (sizeof(char *) * nfile), calceph_strerror_errno(buffer_error));
                ret = 0;
                /* GCOVR_EXCL_STOP */
            }
            else
            {
                int ifile = 0;

                for (kerntmp = kerneltoload->value; kerntmp != NULL && ret != 0; kerntmp = kerntmp->next, ifile++)
                {
                    char *newfilename;
                    size_t lenfilename;
                    struct TXTPCKvalue *kerntmpcont = kerntmp;
                    off_t slen;

                    /* compute the length of the file name */
                    lenfilename = (size_t) (kerntmpcont->loclast - kerntmpcont->locfirst);
                    /* look for the $ statement */
                    ret = calceph_txtmk_symbols_getlen(txtfile.buffer, kerntmpcont, pathsymbols, pathvalues, &slen);
                    if (ret)
                        lenfilename += slen;
                    while (ret != 0 && kerntmpcont != NULL && kerntmpcont->locfirst < kerntmpcont->loclast &&
                           txtfile.buffer[kerntmpcont->loclast - 2] == '+')
                    {
                        kerntmpcont = kerntmpcont->next;
                        if (kerntmpcont)
                        {
                            lenfilename += kerntmpcont->loclast - kerntmpcont->locfirst;
                            /* look for the $ statement */
                            ret =
                                calceph_txtmk_symbols_getlen(txtfile.buffer, kerntmpcont, pathsymbols, pathvalues,
                                                             &slen);
                            if (ret)
                                lenfilename += slen;
                        }
                    }

                    /* allocate memory */
                    newfilename = (char *) malloc(lenfilename * sizeof(char));
                    if (newfilename == NULL)
                    {
                        /* GCOVR_EXCL_START */
                        fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                                   (unsigned long) (sizeof(char) * lenfilename), calceph_strerror_errno(buffer_error));
                        ret = 0;
                        /* GCOVR_EXCL_STOP */
                    }
                    if (ret != 0)
                    {
                        /* create the filename */
                        lenfilename = 0;
                        calceph_txtmk_symbols_copy(newfilename, txtfile.buffer, kerntmp,
                                                   pathsymbols, pathvalues, &lenfilename);
                        while (kerntmp != NULL && kerntmp->locfirst < kerntmp->loclast &&
                               txtfile.buffer[kerntmp->loclast - 2] == '+')
                        {
                            lenfilename--;
                            kerntmp = kerntmp->next;
                            if (kerntmp)
                            {
                                calceph_txtmk_symbols_copy(newfilename, txtfile.buffer, kerntmp,
                                                           pathsymbols, pathvalues, &lenfilename);
                            }
                        }
                        newfilename[lenfilename] = '\0';
#if DEBUG
                        printf("loading '%s'\n", newfilename);
#endif
                        arfilename[ifile] = newfilename;
                    }
                }

                if (ret)
                {
                    /* load the file */
                    t_calcephbin *ephbin = calceph_open_array2(nfile, arfilename);

#if DEBUG
                    printf("calceph_open_array returns '%p'\n", ephbin);
#endif

                    if (ephbin)
                    {
                        if (ephbin->etype != CALCEPH_espice)
                        {
                            fatalerror("Can't open SPICE kernel and INPOP/original DE files at the same time.\n");
                            ret = 0;
                        }
                        /* kernels are spice compatible  => add them to the list of res */
                        else
                        {
                            struct SPICEkernel *prec = res->list;

#if DEBUG
                            printf("res->list : %p %p\n", res->list, &ephbin->data.spkernel);
#endif
                            if (prec == NULL)
                            {
                                *res = ephbin->data.spkernel;
                            }
                            else
                            {
                                while (prec->next != NULL)
                                    prec = prec->next;
                                prec->next = ephbin->data.spkernel.list;
                            }
                            ephbin->data.spkernel.list = NULL;
                            calceph_spice_tablelinkbody_init(&ephbin->data.spkernel.tablelink);
                        }
                        calceph_close(ephbin);
                    }
                    else
                    {
                        ret = 0;
                    }

                }
                /* free memory */
                for (ifile = 0; ifile < nfile; ifile++)
                    free(arfilename[ifile]);
                free(arfilename);
            }

        }
        else
        {
            ret = 0;
        }

        /* unload constants */
#if DEBUG
        printf("unload constants\n");
#endif
        calceph_txtpck_close(&txtfile);
    }

    return ret;

}

/*--------------------------------------------------------------------------*/
/*!  return in slen the length of the symbols ($...) that are present in the string kernel
   if they are several $..., 
   @return  1 on sucess and 0 on failure
 
   @param buffer (in) string. 
   @param kernel (in) position in the string to check.
   @param pathsymbols (in) symbols.   
   @param pathvalues (out) values of the symbols.
   @param slen (out) total length of the symbols' value.
*/
/*--------------------------------------------------------------------------*/
static int calceph_txtmk_symbols_getlen(const char *buffer, const struct TXTPCKvalue *kernel,
                                        const struct TXTPCKconstant *pathsymbols,
                                        const struct TXTPCKconstant *pathvalues, off_t * slen)
{
    int ret = 1;
    off_t pos;

#if DEBUG
    printf("calceph_txtmk_symbols_getlen(,%ld) -enter\n", (long int) *slen);
#endif

    *slen = 0;
    for (pos = kernel->locfirst + 1; ret == 1 && pos <= kernel->loclast - 1; pos++)
    {
        if (buffer[pos] == '$')
        {
            /* look in to pathsymbols */
            const struct TXTPCKvalue *symb = pathsymbols->value;
            const struct TXTPCKvalue *val = pathvalues->value;

            while (val != NULL && symb != NULL && !calceph_txtmk_symbol_cmpeq(buffer, symb, pos + 1))
            {
                symb = symb->next;
                val = val->next;
            }
            if (val == NULL || symb == NULL)
            {
                fatalerror("Can't find a symbol in the kernel.");
                ret = 0;
            }
            else
            {                   /* the symbol was found */
                *slen = *slen + val->loclast - val->locfirst + 1;
            }
        }
    }

#if DEBUG
    printf("calceph_txtmk_symbols_getlen(,%ld) - return %d\n", (long int) *slen, ret);
#endif
    return ret;
}

/*--------------------------------------------------------------------------*/
/*!  return 1 if symb==buffer+loc
 
   @param buffer (in) string 
   @param symb (in) symbol   
   @param loc (in) location to start the check.
*/
/*--------------------------------------------------------------------------*/
static int calceph_txtmk_symbol_cmpeq(const char *buffer, const struct TXTPCKvalue *symb, off_t loc)
{
    int ret = 1;
    off_t j = symb->locfirst + 1;

    while (ret == 1 && j < symb->loclast - 1)
    {
        ret = (buffer[loc++] == buffer[j++]);
    }
    return ret;
}

/*--------------------------------------------------------------------------*/
/*!  copy kernel to the dst 
 
   @param dst (in) destination buffer
   @param buffer (in) string. 
   @param kernel (in) position in the string to check.
   @param pathsymbols (in) symbols.   
   @param pathvalues (out) values of the symbols.
   @param curdst (inout) current location to write in dst
*/
/*--------------------------------------------------------------------------*/
static void calceph_txtmk_symbols_copy(char *dst, const char *buffer, const struct TXTPCKvalue *kernel,
                                       const struct TXTPCKconstant *pathsymbols,
                                       const struct TXTPCKconstant *pathvalues, size_t *curdst)
{
    off_t pos;

    for (pos = kernel->locfirst + 1; pos < kernel->loclast - 1; pos++)
    {
        if (buffer[pos] == '$')
        {
            /* look in to pathsymbols */
            off_t spos;
            const struct TXTPCKvalue *symb = pathsymbols->value;
            const struct TXTPCKvalue *val = pathvalues->value;

            while (val != NULL && symb != NULL && !calceph_txtmk_symbol_cmpeq(buffer, symb, pos + 1))
            {
                symb = symb->next;
                val = val->next;
            }
            pos += (symb->loclast - symb->locfirst - 2);
            /* copy the value of the symbol */
            for (spos = val->locfirst + 1; spos < val->loclast - 1; spos++)
            {
                dst[*curdst] = buffer[spos];
                *curdst = *curdst + 1;
            }

        }
        else
        {
            dst[*curdst] = buffer[pos];
            *curdst = *curdst + 1;
        }
    }

}
