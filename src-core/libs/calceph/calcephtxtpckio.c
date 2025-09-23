/*-----------------------------------------------------------------*/
/*!
  \file calcephtxtpckio.c
  \brief perform I/O on text PCK KERNEL ephemeris data file
         read all constants from a text PCK KERNEL Ephemeris data.

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
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
static int calceph_txtpck_getconstant_intmatrixnxn(struct TXTPCKfile *eph, const char *name, int n1,
                                                   int n2, int *p_pival);
static int calceph_txtpck_getconstant_matrixnxn(struct TXTPCKfile *eph, const char *name, int n1, int n2,
                                                double *p_pdval);

static struct TXTPCKvalue *calceph_txtpck_load_decodevalue(char *buffer, off_t lenfile, off_t begkeyword,
                                                           off_t endkeyword, const char *filename, off_t * pcurpos);
static void calceph_txtpck_load_appendvaluetoend(struct TXTPCKconstant *pnewcst, struct TXTPCKvalue *listvalue);

#if DEBUG
/*--------------------------------------------------------------------------*/
/*!
     Debug the list of constant of a text PCK kernel

  @param header (in) header of text PCK
*/
/*--------------------------------------------------------------------------*/
void calceph_txtpck_debug(const struct TXTPCKfile *header)
{
    struct TXTPCKconstant *listconstant;

    printf("list of constant of the text PCK file : \n");
    for (listconstant = header->listconstant; listconstant != NULL; listconstant = listconstant->next)
    {
        struct TXTPCKvalue *listvalue;

        printf("'%s' = ( ", listconstant->name);
        for (listvalue = listconstant->value; listvalue != NULL; listvalue = listvalue->next)
        {
            printf("%.*s", (int) (listvalue->loclast - listvalue->locfirst), listvalue->buffer + listvalue->locfirst);
            if (listvalue->next != NULL)
                printf(" , ");
        }
        printf(" ) \n");
    }
}
#endif

/*--------------------------------------------------------------------------*/
/*!  Read the constants of the text PCK kernel file

  @return  1 on sucess and 0 on failure

 @param file (inout) file descriptor.
 @param filename (in) A character string giving the name of an ephemeris data file.
 @param locale (in) C locale to handle the decimal point inside the conversion string to double
 @param res (out) descriptor of the ephemeris.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_open(FILE * file, const char *filename, struct calceph_locale clocale, struct SPICEkernel *res)
{
    int ret;

    res->filetype = TXT_PCK;
    ret = calceph_txtpck_load(file, filename, "KPL/PCK", clocale, &res->filedata.txtpck);
    if (ret == 0)
    {
        /* unload constants */
#if DEBUG
        printf("unload constants\n");
#endif
        calceph_txtpck_close(&res->filedata.txtpck);
    }
    return ret;
}

/*--------------------------------------------------------------------------*/
/*!  Read the constants of the text PCK kernel file

  @return  1 on sucess and 0 on failure

 @param file (inout) file descriptor. the file is closed on success
 @param filename (in) A character string giving the name of an ephemeris data file.
 @param header (in) header in the file (e.g., "PKL/MK" or "KPL/PCK")
 @param locale (in) C locale to handle the decimal point inside the conversion string to double
 @param res (out) descriptor of the text file.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_load(FILE * file, const char *filename, const char *header,
                        struct calceph_locale clocale, struct TXTPCKfile *res)
{
    off_t lenfile;

    size_t ulenfile;

    char *buffer;

    off_t curpos;

    const char *begindata = "\\begindata";

    const size_t lenbegindata = strlen(begindata);

    const char *begintext = "\\begintext";

    const size_t lenbegintext = strlen(begintext);

    struct TXTPCKconstant *listconstant = NULL, *precconstant = NULL;

    const size_t lenheader = strlen(header);

    buffer_error_t buffer_error;

    /*---------------------------------------------------------*/
    /* initialize with empty values if a failure occurs during the initialization */
    /*---------------------------------------------------------*/
    res->listconstant = NULL;
    res->buffer = NULL;
    res->clocale = clocale;

    /*---------------------------------------------------------*/
    /* get the file size and read all the file to the memory */
    /*---------------------------------------------------------*/
    /* go to the end of the file */
    if (fseeko(file, 0L, SEEK_END) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to the end of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    lenfile = ftello(file);
    if (lenfile < 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't get the size of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    /* allocate memory */
    lenfile++;
    buffer = (char *) malloc(sizeof(char) * (size_t) lenfile);
    if (buffer == NULL)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                   (unsigned long) (sizeof(char) * (size_t) lenfile), calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    buffer[lenfile - 1] = '\0';

    /* read the file  */
    if (fseeko(file, 0L, SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to the end of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        free(buffer);
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    /* printf("lenfile=%d\n", (int) lenfile); */
    lenfile--;
    ulenfile = (size_t) lenfile;
    if (fread(buffer, sizeof(char), ulenfile, file) != ulenfile)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        free(buffer);
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    /* check the header  */
    if (ulenfile < lenheader || strncmp(buffer, header, lenheader) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read the header of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        free(buffer);
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    /*---------------------------------------------------------*/
    /* loop until the end of file to read the constant */
    /*---------------------------------------------------------*/
    curpos = lenheader;
    while (curpos < lenfile)
    {
        while (curpos < lenfile && buffer[curpos] != '\\')
            curpos++;
        /* look for data section */
        if (curpos + lenbegindata < ulenfile && strncmp(buffer + curpos, begindata, lenbegindata) == 0)
        {
            /* printf("enter in a  data section : '%.50s'\n", buffer + curpos); */
            curpos += lenbegindata;
            /* to skip \begindata inside a text section */
            while (curpos < lenfile && buffer[curpos] != '\n' && buffer[curpos] != '\r' && isspace_char(buffer[curpos]))
                curpos++;
            if (curpos < lenfile && (buffer[curpos] != '\n' && buffer[curpos] != '\r'))
                continue;

            while (curpos < lenfile && buffer[curpos] != '\\')
            {
                while (curpos < lenfile && isspace_char(buffer[curpos]))
                    curpos++;
                if (curpos < lenfile && buffer[curpos] != '\\')
                {
                    struct TXTPCKconstant *pnewcst = NULL;

                    struct TXTPCKvalue *listvalue = NULL;
                    int assignment = 0;

                    /* read the keyword */
                    off_t begkeyword = curpos;

                    off_t endkeyword;

                    size_t sizekeyword; /* in characters */

                    while (curpos < lenfile && !isspace_char(buffer[curpos]) && buffer[curpos] != '=')
                        curpos++;
                    endkeyword = curpos;
                    sizekeyword = endkeyword - begkeyword;
                    /* printf("keyword= %d %d '%.*s'\n", (int) begkeyword, (int) endkeyword, (int) (endkeyword -
                     * begkeyword), buffer + begkeyword); */

                    while (curpos < lenfile && isspace_char(buffer[curpos]))
                        curpos++;
                    if (curpos + 1 < lenfile)
                    {
                        if (buffer[curpos] == '+' && buffer[curpos + 1] == '=')
                        {
                            curpos += 2;
                            assignment = 1;
                        }
                        else if (buffer[curpos] == '=')
                        {
                            curpos++;
                            assignment = 0;
                        }
                        else
                        {
                            /* GCOVR_EXCL_START */
                            fatalerror("The file '%s' is malformed\n", filename);
                            free(buffer);
                            return 0;
                            /* GCOVR_EXCL_STOP */
                        }
                    }
                    else
                    {
                        /* GCOVR_EXCL_START */
                        fatalerror("The file '%s' is malformed\n", filename);
                        free(buffer);
                        return 0;
                        /* GCOVR_EXCL_STOP */
                    }

                    /* decode the value */
                    listvalue = calceph_txtpck_load_decodevalue(buffer, lenfile, begkeyword, endkeyword, filename,
                                                                &curpos);
                    if (listvalue == NULL)
                    {
                        free(buffer);
                        return 0;
                    }

                    /* incremental assignment : try to find if the same constant exists */
                    if (assignment == 1)
                    {
                        for (pnewcst = listconstant; pnewcst != NULL; pnewcst = pnewcst->next)
                        {
                            if (strlen(pnewcst->name) == sizekeyword
                                && memcmp(pnewcst->name, buffer + begkeyword, sizekeyword) == 0)
                            {
                                /* constant already exist : push the value at the end */
                                calceph_txtpck_load_appendvaluetoend(pnewcst, listvalue);
                                break;
                            }
                        }
                    }

                    if (pnewcst == NULL)
                    {
                        /*create the new constant */
                        pnewcst = (struct TXTPCKconstant *) malloc(sizeof(struct TXTPCKconstant));
                        if (pnewcst == NULL)
                        {
                            /* GCOVR_EXCL_START */
                            fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                                       (unsigned long) (sizeof(struct TXTPCKconstant)),
                                       calceph_strerror_errno(buffer_error));
                            free(buffer);
                            return 0;
                            /* GCOVR_EXCL_STOP */
                        }
                        pnewcst->next = NULL;
                        pnewcst->value = listvalue;
                        pnewcst->assignment = assignment;
                        pnewcst->name = (char *) malloc((sizekeyword + 1) * sizeof(char));
                        if (pnewcst->name == NULL)
                        {
                            /* GCOVR_EXCL_START */
                            fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                                       (unsigned long) (sizeof(sizekeyword + 1)), calceph_strerror_errno(buffer_error));
                            free(buffer);
                            return 0;
                            /* GCOVR_EXCL_STOP */
                        }
                        memcpy(pnewcst->name, buffer + begkeyword, sizekeyword * sizeof(char));
                        pnewcst->name[endkeyword - begkeyword] = '\0';
                        if (precconstant == NULL)
                        {
                            precconstant = listconstant = pnewcst;
                            res->listconstant = listconstant;
                        }
                        else
                        {
                            precconstant->next = pnewcst;
                            precconstant = precconstant->next;
                        }
                    }
                }
                if (curpos + lenbegintext < ulenfile && strncmp(buffer + curpos, begintext, lenbegintext) == 0)
                {
                    /* printf("start of a text section : %d\n", (int) curpos); */
                    curpos += lenbegintext;
                    break;      /* end the data section */
                }
            }
        }
        curpos++;
    }

    res->listconstant = listconstant;
    res->buffer = buffer;
    res->clocale = clocale;

    fclose(file);
    return 1;
}

/*--------------------------------------------------------------------------*/
/*! append listvalue to the end of values of pnewcst
 @param pnewcst (in) contant to update
 @param listvalue (in) list value to move
*/
/*--------------------------------------------------------------------------*/
void calceph_txtpck_load_appendvaluetoend(struct TXTPCKconstant *pnewcst, struct TXTPCKvalue *listvalue)
{
    if (pnewcst->value == NULL)
    {
        pnewcst->value = listvalue;
    }
    else
    {
        struct TXTPCKvalue *taillistvalue = pnewcst->value;

        while (taillistvalue->next != NULL)
            taillistvalue = taillistvalue->next;
        taillistvalue->next = listvalue;
    }
}

/*--------------------------------------------------------------------------*/
/*!  Read the value associated to a constant of the text PCK kernel file

  It decodes the value after the symbol '=' or '+='

  @return NULL on failure, otherwise the list of values

 @param buffer (in) buffer of the text kernel.
 @param lenfile (in) size of buffer.
 @param begkeyword (in) position of the first character of the keyword in the buffer
 @param endkeyword (in) next position of the last character of the keyword in the buffer
 @param filename (in) A character string giving the name of an ephemeris data file.
 @param pcurpos (inout) current position of the parser.
*/
/*--------------------------------------------------------------------------*/
struct TXTPCKvalue *calceph_txtpck_load_decodevalue(char *buffer, off_t lenfile, off_t begkeyword,
                                                    off_t endkeyword, const char *filename, off_t * pcurpos)
{
    off_t curpos = *pcurpos;
    struct TXTPCKvalue *listvalue = NULL, *prec = NULL;
    off_t begvalue, endvalue;

    buffer_error_t buffer_error;

    /* find the ( or ' or digit or sign +- */
    while (curpos < lenfile && isspace_char(buffer[curpos]))
        curpos++;

    if (curpos >= lenfile)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't find  the character '(' , ' or a digit after the keyword '%.*s'\n",
                   (int)(endkeyword - begkeyword), buffer + begkeyword);
        return NULL;
        /* GCOVR_EXCL_STOP */
    }

   /* check the parenthesis or ' or digit or sign +- */
    if (buffer[curpos] != '(' && buffer[curpos] != '\'' && !isdigit_char(buffer[curpos])
        && buffer[curpos] != '+' && buffer[curpos] != '-')
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't find  the character '(' , ' or a digit after the keyword '%.*s'\n",
                   (int)(endkeyword - begkeyword), buffer + begkeyword);
        return NULL;
        /* GCOVR_EXCL_STOP */
    }
    /* single string */
    if (buffer[curpos] == '\'')
    {
        begvalue = curpos;
        curpos++;
        while (curpos < lenfile && buffer[curpos] != '\'')
            curpos++;
        curpos++;
        endvalue = curpos;
        /* create the new value */
        listvalue = (struct TXTPCKvalue *) malloc(sizeof(struct TXTPCKvalue));
        if (listvalue == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                       (unsigned long) (sizeof(struct TXTPCKvalue)), calceph_strerror_errno(buffer_error));
            return NULL;
            /* GCOVR_EXCL_STOP */
        }
        listvalue->next = NULL;
        listvalue->buffer = buffer;
        listvalue->locfirst = begvalue;
        listvalue->loclast = endvalue;
    }
    else if (buffer[curpos] == '(')
    {
        curpos++;               /* skip parenthesis */
        while (curpos < lenfile && buffer[curpos] != ')')
        {
            struct TXTPCKvalue *pnewval;

            /* read the value */
            while (curpos < lenfile && isspace_char(buffer[curpos]))
                curpos++;
            begvalue = curpos;

            if (curpos < lenfile && buffer[curpos] == '\'')
            {                   /* begin a string */
                curpos++;
                while (curpos < lenfile
                       && (buffer[curpos] != '\''
                           || (buffer[curpos] == '\'' && curpos + 1 < lenfile && buffer[curpos + 1] == '\'')))
                {
                    if (buffer[curpos] == '\'')
                        curpos++;
                    curpos++;
                }
                curpos++;
                endvalue = curpos;
            }
            else
            {                   /* begin a value */
                off_t k;

                while (curpos < lenfile && !isspace_char(buffer[curpos]) && buffer[curpos] != ','
                       && buffer[curpos] != ')')
                {
                    curpos++;
                }
                endvalue = curpos;
                /*replace fortran number by C number */
                for (k = begvalue; k <= endvalue; k++)
                {
                    if (buffer[k] == 'D' || buffer[k] == 'd')
                        buffer[k] = 'E';
                }
            }

            /* create the new value */
            pnewval = (struct TXTPCKvalue *) malloc(sizeof(struct TXTPCKvalue));
            if (pnewval == NULL)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                           (unsigned long) (sizeof(struct TXTPCKvalue)), calceph_strerror_errno(buffer_error));
                return NULL;
                /* GCOVR_EXCL_STOP */
            }
            pnewval->next = NULL;
            pnewval->buffer = buffer;
            pnewval->locfirst = begvalue;
            pnewval->loclast = endvalue;
            if (prec == NULL)
                listvalue = prec = pnewval;
            else
            {
                prec->next = pnewval;
                prec = pnewval;
            }

            /* find the ) or , */
            while (curpos < lenfile && isspace_char(buffer[curpos]))
                curpos++;

            if (curpos < lenfile && buffer[curpos] == ',')
                curpos++;
        }
        curpos++;               /* skip parenthesis */
    }
    /* digit case */
    else if (isdigit_char(buffer[curpos]) || buffer[curpos] == '+' || buffer[curpos] == '-')
    {
        struct TXTPCKvalue *pnewval;

        off_t k;

        /* begin a value */
        begvalue = curpos;
        while (curpos < lenfile
               && (isdigit_char(buffer[curpos]) || buffer[curpos] == '.' || buffer[curpos] == 'D'
                   || buffer[curpos] == 'd' || buffer[curpos] == 'E' || buffer[curpos] == '+' || buffer[curpos] == '-'))
        {
            curpos++;
        }
        endvalue = curpos;
        /*replace fortran number by C number */
        for (k = begvalue; k <= endvalue; k++)
        {
            if (buffer[k] == 'D' || buffer[k] == 'd')
                buffer[k] = 'E';
        }

        /* create the new value */
        pnewval = (struct TXTPCKvalue *) malloc(sizeof(struct TXTPCKvalue));
        if (pnewval == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                       (unsigned long) (sizeof(struct TXTPCKvalue)), calceph_strerror_errno(buffer_error));
            return NULL;
            /* GCOVR_EXCL_STOP */
        }
        pnewval->next = NULL;
        pnewval->buffer = buffer;
        pnewval->locfirst = begvalue;
        pnewval->loclast = endvalue;
        if (prec == NULL)
            listvalue = prec = pnewval;
        else
        {
            prec->next = pnewval;
            prec = pnewval;
        }
    }
    else
    {
        fatalerror("unknown value type  after the keyword '%.*s' in the file '%s'\n",
                   (int)(endkeyword - begkeyword), buffer + begkeyword, filename);
        return NULL;
    }

    *pcurpos = curpos;
    return listvalue;
}

/*--------------------------------------------------------------------------*/
/*
 * ! Close the  text PCK kernel file
 *
 *
 * @param eph (inout) descriptor of the ephemeris.
 */
/*--------------------------------------------------------------------------*/
void calceph_txtpck_close(struct TXTPCKfile *eph)
{
    struct TXTPCKconstant *listconstant = eph->listconstant;

    while (listconstant != NULL)
    {
        struct TXTPCKconstant *pcst = listconstant;

        struct TXTPCKvalue *listvalue = listconstant->value;

        free(listconstant->name);

        while (listvalue != NULL)
        {
            struct TXTPCKvalue *pvalue = listvalue;

            listvalue = listvalue->next;
            free(pvalue);
        }
        listconstant = listconstant->next;
        free(pcst);
    }

    if (eph->buffer)
        free(eph->buffer);

    eph->listconstant = NULL;
    eph->buffer = NULL;
}

/*--------------------------------------------------------------------------*/
/*! fill the array arrayvalue with the nvalue first values of the specified constant from the text PCK
  kernel return 0 if constant is not found return the number of values associated to the constant (this
  value may be larger than nvalue), if constant is  found
  @param eph (in) ephemeris file
  @param name (in) constant name
  @param name (in) constant name
  @param arrayvalue (out) array of values of the constant.
    On input, the size of the array is nvalue elements.
    On exit, the min(returned value, nvalue) elements are filled.
  @param nvalue (in) number of elements in the array arrayvalue.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_vd(const struct TXTPCKfile *eph, const char *name, double *arrayvalue, int nvalue)
{
    struct TXTPCKconstant *listconstant;

    int found = 0;

    for (listconstant = eph->listconstant; listconstant != NULL && found == 0; listconstant = listconstant->next)
    {
        if (strcmp(listconstant->name, name) == 0)
        {
            struct TXTPCKvalue *listvalue;

            for (listvalue = listconstant->value; listvalue != NULL; listvalue = listvalue->next)
            {
                if (listvalue->buffer[listvalue->locfirst] != '\'')
                {
                    if (found < nvalue)
                    {
                        char *perr;

                        arrayvalue[found] = calceph_strtod(listvalue->buffer + listvalue->locfirst, &perr,
                                                           eph->clocale);
                        found += (((off_t) (perr - listvalue->buffer)) <= listvalue->loclast) ? 1 : 0;
                    }
                    else
                    {
                        found++;
                    }
                }
            }
        }
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pival.
  @param listconstant (in) list of constants
  @param p_pival (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_int2(const struct TXTPCKconstant *listconstant, int *p_pival)
{
    int found = 0;

    struct TXTPCKvalue *listvalue;

    for (listvalue = listconstant->value; listvalue != NULL && found == 0; listvalue = listvalue->next)
    {
        if (listvalue->buffer[listvalue->locfirst] != '\'')
        {
            char *perr;

            long l = strtol(listvalue->buffer + listvalue->locfirst, &perr, 10);

            *p_pival = (int) l;
            found = (((off_t) (perr - listvalue->buffer)) <= listvalue->loclast);
        }
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pival.
  @param eph (inout) ephemeris file
  @param name (in) constant name
  @param p_pival (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_int(struct TXTPCKfile *eph, const char *name, int *p_pival)
{
    struct TXTPCKconstant *listconstant;

    int found = 0;

    for (listconstant = eph->listconstant; listconstant != NULL && found == 0; listconstant = listconstant->next)
    {
        if (strcmp(listconstant->name, name) == 0)
        {
            found = calceph_txtpck_getconstant_int2(listconstant, p_pival);
        }
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pdval.
  @param eph (inout) ephemeris file
  @param n1 (in) 1st dimension of the matrix
  @param n2 (in) 2nd dimension of the matrix
  @param name (in) constant name
  @param p_pival (out) matrix n1xn2 : value of the constant
*/
/*--------------------------------------------------------------------------*/
static int calceph_txtpck_getconstant_intmatrixnxn(struct TXTPCKfile *eph, const char *name, int n1,
                                                   int n2, int *p_pival)
{
    int j;

    int n = n1 * n2;

    struct TXTPCKconstant *ptr;

    ptr = calceph_txtpck_getptrconstant(eph, name);
    if (ptr)
    {
        int found = 1;

        struct TXTPCKvalue *listvalue;

        j = 0;
        for (listvalue = ptr->value; listvalue != NULL && found == 1 && j < n; listvalue = listvalue->next, j++)
        {
            if (listvalue->buffer[listvalue->locfirst] != '\'')
            {
                char *perr;

                long l = strtol(listvalue->buffer + listvalue->locfirst, &perr, 10);

                p_pival[j] = (int) l;

                found = (isspace_char(*perr) || *perr == ',')
                    && (((off_t) (perr - listvalue->buffer)) <= listvalue->loclast);
            }
            else
            {
                found = 0;
            }
        }
        if (j != n)
            found = 0;
        return found;
    }
    return 0;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pdval.
  @param eph (inout) ephemeris file
  @param name (in) constant name
  @param p_pival (out) matrix 1x3 : value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_intmatrix1x3(struct TXTPCKfile *eph, const char *name, int *p_pival)
{
    return calceph_txtpck_getconstant_intmatrixnxn(eph, name, 1, 3, p_pival);
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pdval.
  @param eph (inout) ephemeris file
  @param n1 (in) 1st dimension of the matrix
  @param n2 (in) 2nd dimension of the matrix
  @param name (in) constant name
  @param p_pdval (out) matrix n1xn2 : value of the constant
*/
/*--------------------------------------------------------------------------*/
static int calceph_txtpck_getconstant_matrixnxn(struct TXTPCKfile *eph, const char *name, int n1, int n2,
                                                double *p_pdval)
{
    int j;

    int n = n1 * n2;

    struct TXTPCKconstant *ptr;

    ptr = calceph_txtpck_getptrconstant(eph, name);
    if (ptr)
    {
        int found = 1;

        struct TXTPCKvalue *listvalue;

        j = 0;
        for (listvalue = ptr->value; listvalue != NULL && found == 1 && j < n; listvalue = listvalue->next, j++)
        {
            if (listvalue->buffer[listvalue->locfirst] != '\'')
            {
                char *perr;

                p_pdval[j] = calceph_strtod(listvalue->buffer + listvalue->locfirst, &perr, eph->clocale);
                found = (isspace_char(*perr) || *perr == ',')
                    && (((off_t) (perr - listvalue->buffer)) <= listvalue->loclast);
            }
            else
            {
                found = 0;
            }
        }
        if (j != n)
            found = 0;
        return found;
    }
    return 0;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pdval.
  @param eph (inout) ephemeris file
  @param name (in) constant name
  @param p_pdval (out) matrix 3x3 : value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_matrix3x3(struct TXTPCKfile *eph, const char *name, double *p_pdval)
{
    return calceph_txtpck_getconstant_matrixnxn(eph, name, 3, 3, p_pdval);
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pdval.
  @param eph (inout) ephemeris file
  @param name (in) constant name
  @param p_pdval (out) matrix 1x3 : value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_matrix1x3(struct TXTPCKfile *eph, const char *name, double *p_pdval)
{
    return calceph_txtpck_getconstant_matrixnxn(eph, name, 1, 3, p_pdval);
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  the value must be a string
  return 0 if constant is not found
  return 1 if constant is found and set p_ptxtpckvalue.
  @param eph (inout) ephemeris file
  @param name (in) constant name
  @param p_ptxtpckvalue (out) value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstant_txtpckvalue(struct TXTPCKfile *eph, const char *name,
                                           struct TXTPCKvalue **p_ptxtpckvalue)
{
    struct TXTPCKconstant *listconstant;

    int found = 0;

    *p_ptxtpckvalue = NULL;

    for (listconstant = eph->listconstant; listconstant != NULL && found == 0; listconstant = listconstant->next)
    {
        if (strcmp(listconstant->name, name) == 0)
        {
            struct TXTPCKvalue *listvalue;

            for (listvalue = listconstant->value; listvalue != NULL && found == 0; listvalue = listvalue->next)
            {
                if (listvalue->buffer[listvalue->locfirst] == '\'')
                {
                    *p_ptxtpckvalue = listvalue;
                    found = 1;
                }
            }
        }
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified constant from the text PCK kernel
  return NULL if constant is not found
  return the address of the constant if constant is found.

  @param eph (inout) ephemeris file
  @param name (in) constant name
*/
/*--------------------------------------------------------------------------*/
struct TXTPCKconstant *calceph_txtpck_getptrconstant(struct TXTPCKfile *eph, const char *name)
{
    struct TXTPCKconstant *foundconstant = NULL;

    struct TXTPCKconstant *listconstant;

    for (listconstant = eph->listconstant; listconstant != NULL && foundconstant == NULL;
         listconstant = listconstant->next)
    {
        if (strcmp(listconstant->name, name) == 0)
        {
            foundconstant = listconstant;
        }
    }
    return foundconstant;
}

/*--------------------------------------------------------------------------*/
/*! return the number of constants available in the ephemeris file

  @param eph (inout) ephemeris descriptor
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstantcount(struct TXTPCKfile *eph)
{
    int res = 0;

    struct TXTPCKconstant *listconstant;

    for (listconstant = eph->listconstant; listconstant != NULL; listconstant = listconstant->next)
    {
        res++;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! get the name and its first value of the constant available
    at some index from the ephemeris file
   return the number of values associated to the constant,
     if index is validd and set value to the first value.
   return 0 if index isn't valid

  @param eph (inout) ephemeris descriptor
  @param index (in) index of the constant (must be between 1 and calceph_getconstantcount() )
   if index > number of constant in this file => decrement it
  @param name (out) name of the constant
  @param value (out) first value of the constant
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getconstantindex(struct TXTPCKfile *eph, int *index,
                                    char name[CALCEPH_MAX_CONSTANTNAME], double *value)
{
    struct TXTPCKconstant *listconstant = eph->listconstant;

    int res = 0;

    while (listconstant != NULL && *index > 1)
    {
        *index = *index - 1;
        listconstant = listconstant->next;
    }

    if (listconstant != NULL && *index == 1)
    {
        struct TXTPCKvalue *listvalue = listconstant->value;

        strcpy(name, listconstant->name);

        for (; listvalue != NULL; listvalue = listvalue->next)
        {
            if (listvalue->buffer[listvalue->locfirst] != '\'')
            {
                if (res == 0)
                {
                    char *perr;

                    *value = calceph_strtod(listvalue->buffer + listvalue->locfirst, &perr, eph->clocale);
                    res = (((off_t) (perr - listvalue->buffer)) <= listvalue->loclast) ? 1 : 0;
                    /*printf("dvalue=%g found=%d %d %d\n", *value, res, (int)(perr-eph->buffer),
                     * (int)listvalue->loclast); */
                }
                else
                {
                    res++;
                }
            }
        }
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! return 0 if the value1==value2
    return -1 or +1 if not the same
   @param value1 (in) first value
   @param value2 (in) second value
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_cmpvalue(const struct TXTPCKvalue *value1, const struct TXTPCKvalue *value2)
{
    off_t loc1 = value1->locfirst;

    off_t loc2 = value2->locfirst;

    while (loc1 <= value1->loclast && loc2 <= value2->loclast)
    {
        if (value1->buffer[loc1] != value2->buffer[loc2])
            return 1;
        loc1++;
        loc2++;
    }

    return (loc1 > value1->loclast && loc2 > value2->loclast) ? 0 : 1;
}

/*--------------------------------------------------------------------------*/
/*! return 0 if the value1==value2
    return -1 or +1 if not the same
   @param value1 (in) first value
   @param value2 (in) second value
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_cmpszvalue(struct TXTPCKvalue *value1, const char *value2)
{
    off_t loc1 = value1->locfirst;

    while (loc1 <= value1->loclast && *value2 != '\0')
    {
        if (value1->buffer[loc1] != *value2)
            return 1;
        loc1++;
        value2++;
    }
    return (loc1 >= value1->loclast && *value2 == '\0') ? 0 : 1;
}

/*--------------------------------------------------------------------------*/
/*! return the value of the specified value from the text PCK kernel
  return 0 if constant is not found
  return 1 if constant is found and set p_pival.
  @param listvalue (in) list of values
  @param p_pszval (inout) string of the value.
  on enter, the array must have enough room.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_getvalue_char(struct TXTPCKvalue *listvalue, char *p_pszval)
{
    int found = 0;

    off_t l, j;

    p_pszval[0] = '\0';
    if (listvalue->buffer[listvalue->locfirst] == '\'')
    {
        j = 0;
        for (l = listvalue->locfirst + 1; l < listvalue->loclast - 1; l++)
        {
            p_pszval[j++] = listvalue->buffer[l];
        }
        p_pszval[j] = '\0';
        found = 1;
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! return the string converted as double using the given locale
  @param nptr (in) string to convert
  @param nptr (inout) last character used by the conversion
  @param clocale (in) C locale to handle the decimal point
  on enter, the array must have enough room.
*/
/*--------------------------------------------------------------------------*/
double calceph_strtod(const char *nptr, char **endptr, struct calceph_locale clocale)
{
#if USE_STRTOD_L
    if (clocale.useclocale == 1)
    {
        return strtod_l(nptr, endptr, clocale.dataclocale);
    }
    else
#endif
    {
        (void) clocale;
        return strtod(nptr, endptr);
    }
}

/*--------------------------------------------------------------------------*/
/*! merge the incremental assignment from newkernel to the existing constants from the list

   @return  1 on sucess and 0 on failure

 @param list (inout) exiting list to update, except the element equals to newkernel
 @param newkernel (in) take increment assigment from this list
*/
/*--------------------------------------------------------------------------*/
int calceph_txtpck_merge_incrementalassignment(struct SPICEkernel *list, struct SPICEkernel *newkernel)
{
    struct TXTPCKconstant *prec = NULL, *cur = NULL, *nextcur = NULL, *nextprec = NULL;

    if (newkernel->filetype != TXT_PCK)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Internal error in calceph_txtpck_merge_incrementalassignment : type of the kernel "
                   "should be TXT_PCK");
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    for (cur = newkernel->filedata.txtpck.listconstant; cur != NULL;)
    {
        nextcur = cur->next;
        nextprec = cur;
        if (cur->assignment == 1)
        {
            /* find if the constant exit in the previous list */
            struct SPICEkernel *ker;

            for (ker = list; ker != NULL && cur != NULL; ker = ker->next)
            {
                if (ker != newkernel && ker->filetype == TXT_PCK)
                {
                    struct TXTPCKconstant *existingconstant
                        = calceph_txtpck_getptrconstant(&(ker->filedata.txtpck), cur->name);
                    if (existingconstant != NULL)
                    {
                        /* move the value from newkernel to existingconstant */
                        calceph_txtpck_load_appendvaluetoend(existingconstant, cur->value);
                        cur->value = NULL;
                        /* remove this constant from newkernel */
                        nextprec = prec;
                        if (prec == NULL)
                        {
                            newkernel->filedata.txtpck.listconstant = nextcur;
                        }
                        else
                        {
                            prec->next = nextcur;
                        }
                        free(cur->name);
                        free(cur);
                        cur = NULL;
                    }
                }
            }
        }
        cur = nextcur;
        prec = nextprec;
    }

    return 1;
}
