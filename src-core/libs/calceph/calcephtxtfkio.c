/*-----------------------------------------------------------------*/
/*!
  \file calcephtxtfkio.c
  \brief perform I/O on text Frame  KERNEL (FK) ephemeris data file
         read all constants from a text Frame KERNEL (FK)  Ephemeris data.

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
/* private functions */
/*--------------------------------------------------------------------------*/
#if DEBUG
static void calceph_txtfk_debug(const struct TXTFKfile *header);
#endif
static int calceph_txtfk_convert_eulerangles(double tkframe_angles[3], struct TXTPCKvalue *tkframe_unit);

static void calceph_txtfk_creatematrix_axes2(double matrix[3][3], const double A);

static void calceph_txtfk_creatematrix_axes3(double matrix[3][3], const double A);

static int calceph_txtfk_creatematrix_axesk(double matrix[3][3], int k, const double A);

#if DEBUG
/*--------------------------------------------------------------------------*/
/*!
     Debug the list of constant of a text FK kernel

  @param header (in) header of text FK
*/
/*--------------------------------------------------------------------------*/
static void calceph_txtfk_debug(const struct TXTFKfile *header)
{
    struct TXTFKframe *listframe;

    calceph_txtpck_debug(&header->txtpckfile);
    printf("list of frame of the text FK file : \n");
    for (listframe = header->listframe; listframe != NULL; listframe = listframe->next)
    {
        printf("'%s' = ( ' ', %d, class=%2d, classid=%8d center = %8d ", listframe->name,
               /*listframe->frame_name, */ listframe->frame_id, listframe->classtype, listframe->class_id,
               listframe->center);

        printf(" ) \n");
    }
    printf("end of list of frame of the text FK file\n");
}
#endif

/*--------------------------------------------------------------------------*/
/*!  Read the constants of the text Frame kernel (FK) file

   @return  1 on sucess and 0 on failure

   @param file (inout) file descriptor. Closed on exit if success.
   @param filename (in) A character string giving the name of an ephemeris data file.
 @param locale (in) C locale to handle the decimal point inside the conversion string to double
   @param res (out) descriptor of the ephemeris.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtfk_open(FILE * file, const char *filename, struct calceph_locale clocale, struct SPICEkernel *res)
{
    buffer_error_t buffer_error;

    off_t lenfile;

    size_t ulenfile;

    char *buffer;

    off_t curpos;

    const char *begindata = "\\begindata";

    const size_t lenbegindata = strlen(begindata);

    const char *begintext = "\\begintext";

    const size_t lenbegintext = strlen(begintext);

    struct TXTPCKconstant *listconstant = NULL, *precconstant = NULL;

    struct TXTFKframe *rootlistframe;

  /*---------------------------------------------------------*/
    /* initialize with empty values if a failure occurs during the initialization */
  /*---------------------------------------------------------*/
    res->filetype = TXT_FK;
    res->filedata.txtfk.txtpckfile.listconstant = NULL;
    res->filedata.txtfk.txtpckfile.buffer = NULL;
    res->filedata.txtfk.txtpckfile.clocale = clocale;
    res->filedata.txtfk.listframe = NULL;

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
        fatalerror("Can't get the size of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        return 0;
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
        fatalerror("Can't jump to the end of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        return 0;
    }
    /* printf("lenfile=%d\n", (int) lenfile); */
    lenfile--;
    ulenfile = (size_t) lenfile;
    if (fread(buffer, sizeof(char), ulenfile, file) != ulenfile)
    {
        fatalerror("Can't read the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        return 0;
    }
    /* check the header  */
    if (lenfile < 6 || strncmp(buffer, "KPL/FK", 6) != 0)
    {
        fatalerror("Can't read the header of the ephemeris file '%s'\nSystem error : '%s'\n", filename,
                   calceph_strerror_errno(buffer_error));
        return 0;
    }
  /*---------------------------------------------------------*/
    /* loop until the end of file to read the constant */
  /*---------------------------------------------------------*/
    curpos = 7;
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
                    struct TXTPCKconstant *pnewcst;

                    struct TXTPCKvalue *listvalue = NULL, *prec = NULL;

                    /* read the keyword : 
                       contain any characters except : tab, space, comma, parenthese, equal */
                    off_t begkeyword = curpos;

                    off_t endkeyword;

                    off_t begvalue, endvalue;

                    while (curpos < lenfile && !isspace_char(buffer[curpos]) && buffer[curpos] != '='
                           && buffer[curpos] != ',' && buffer[curpos] != '(')
                        curpos++;
                    endkeyword = curpos;
                    /* printf("keyword= %d %d '%.*s'\n", (int) begkeyword, (int) endkeyword, (int) (endkeyword -
                     * begkeyword), buffer + begkeyword); */
                    while (curpos < lenfile
                           && (isspace_char(buffer[curpos]) || buffer[curpos] == '=' || buffer[curpos] == '+'))
                        curpos++;

                    /* find the ( */
                    while (curpos < lenfile && isspace_char(buffer[curpos]))
                        curpos++;

                    if (curpos>=lenfile)
                    {
                        fatalerror("Can't find  the character '(' or '  or a digit after the keyword '%.*s'\n",
                                   (int)(endkeyword - begkeyword), buffer + begkeyword);
                        return 0;
                    }

                    /* check the parenthesis */
                    if (buffer[curpos] != '(' && buffer[curpos] != '\'' && !isdigit_char(buffer[curpos])
                        && buffer[curpos] != '-')
                    {
                        fatalerror("Can't find  the character '(' or '  or a digit after the keyword '%.*s'\n",
                                   (int)(endkeyword - begkeyword), buffer + begkeyword);
                        return 0;
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
                                       (unsigned long) (sizeof(struct TXTPCKvalue)),
                                       calceph_strerror_errno(buffer_error));
                            return 0;
                            /* GCOVR_EXCL_STOP */
                        }
                        listvalue->next = NULL;
                        listvalue->buffer = buffer;
                        listvalue->locfirst = begvalue;
                        listvalue->loclast = endvalue;
                    }
                    else if (buffer[curpos] == '(')
                    {
                        curpos++;   /* skip parenthesis */
                        while (curpos < lenfile && buffer[curpos] != ')')
                        {
                            struct TXTPCKvalue *pnewval;

                            /* read the value */
                            while (curpos < lenfile && isspace_char(buffer[curpos]))
                                curpos++;
                            begvalue = curpos;

                            if (curpos < lenfile && buffer[curpos] == '\'')
                            {   /* begin a string */
                                curpos++;
                                while (curpos < lenfile && buffer[curpos] != '\'')
                                    curpos++;
                                curpos++;
                                endvalue = curpos;
                            }
                            else
                            {   /* begin a value */
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
                                           (unsigned long) (sizeof(struct TXTPCKvalue)),
                                           calceph_strerror_errno(buffer_error));
                                return 0;
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
                        curpos++;   /* skip parenthesis */
                    }
                    /* a number */
                    else
                    {
                        begvalue = curpos;
                        curpos++;
                        while (curpos < lenfile && !isspace_char(buffer[curpos]))
                            curpos++;
                        curpos++;
                        endvalue = curpos;
                        /* create the new value */
                        listvalue = (struct TXTPCKvalue *) malloc(sizeof(struct TXTPCKvalue));
                        if (listvalue == NULL)
                        {
                            fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                                       (unsigned long) (sizeof(struct TXTPCKvalue)),
                                       calceph_strerror_errno(buffer_error));
                            return 0;
                        }
                        listvalue->next = NULL;
                        listvalue->buffer = buffer;
                        listvalue->locfirst = begvalue;
                        listvalue->loclast = endvalue;
                    }

                    /* create the new constant */
                    pnewcst = (struct TXTPCKconstant *) malloc(sizeof(struct TXTPCKconstant));
                    if (pnewcst == NULL)
                    {
                        /* GCOVR_EXCL_START */
                        fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                                   (unsigned long) (sizeof(struct TXTPCKconstant)),
                                   calceph_strerror_errno(buffer_error));
                        return 0;
                        /* GCOVR_EXCL_STOP */
                    }
                    pnewcst->next = NULL;
                    pnewcst->value = listvalue;
                    pnewcst->assignment = 0;
                    pnewcst->name = (char *) malloc(((size_t) (endkeyword - begkeyword + 1)) * sizeof(char));
                    if (pnewcst->name == NULL)
                    {
                        /* GCOVR_EXCL_START */
                        fatalerror("Can't allocate memory for %lu bytes\nSystem error : '%s'\n",
                                   (unsigned long) (sizeof(endkeyword - begkeyword + 1)),
                                   calceph_strerror_errno(buffer_error));
                        return 0;
                        /* GCOVR_EXCL_STOP */
                    }
                    memcpy(pnewcst->name, buffer + begkeyword, (endkeyword - begkeyword) * sizeof(char));
                    pnewcst->name[endkeyword - begkeyword] = '\0';
                    if (precconstant == NULL)
                        precconstant = listconstant = pnewcst;
                    else
                    {
                        precconstant->next = pnewcst;
                        precconstant = precconstant->next;
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

    res->filedata.txtfk.txtpckfile.listconstant = listconstant;
    res->filedata.txtfk.txtpckfile.buffer = buffer;
    res->filedata.txtfk.txtpckfile.clocale = clocale;
    rootlistframe = NULL;

  /*---------------------------------------------------------*/
    /* parse the frames */
  /*---------------------------------------------------------*/
    while (listconstant != NULL)
    {
        /* check that the first constant is a frame name */
        if (strncmp(listconstant->name, "FRAME_", 6) == 0)
        {
            char consttmpname[512];

            /* allocate memory for the frame */
            struct TXTFKframe *newframe;

            newframe = (struct TXTFKframe *) malloc(sizeof(struct TXTFKframe));
            if (newframe == NULL)
            {
                /* GCOVR_EXCL_START */
                fatalerror("Can't allocate memory for %d bytes\n", (int) sizeof(struct TXTFKframe));
                calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                return 0;
                /* GCOVR_EXCL_STOP */
            }
            /* find the TK frame */
            newframe->tkframe_relative = NULL;
            newframe->tkframe_spec = NULL;
            newframe->tkframe_unit = NULL;
            newframe->tkframe_quaternion = NULL;
            newframe->tkframe_relative_id = 0;
            /* add the new frame to the list */
            if (rootlistframe)
            {
                rootlistframe->next = newframe;
            }
            else
            {
                res->filedata.txtfk.listframe = newframe;
            }
            newframe->next = NULL;
            rootlistframe = newframe;

            /* get the value : frame number */
            newframe->name = listconstant->name;
            if (calceph_txtpck_getconstant_int(&res->filedata.txtfk.txtpckfile, newframe->name,
                                               &newframe->frame_id) == 0)
            {
                fatalerror("Can't find the frame id of the frame  '%s'\n", listconstant->name);
                calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                return 0;
            }
            calceph_snprintf(consttmpname, 512, "FRAME_%d_NAME", newframe->frame_id);
            if (calceph_txtpck_getconstant_txtpckvalue(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                       &newframe->frame_name) == 0)
            {
                fatalerror("Can't find the name of the frame  '%s' (%s)\n", newframe->name, consttmpname);
                calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                return 0;
            }
            calceph_snprintf(consttmpname, 512, "FRAME_%d_CLASS", newframe->frame_id);
            if (calceph_txtpck_getconstant_int(&res->filedata.txtfk.txtpckfile, consttmpname,
                                               &newframe->classtype) == 0)
            {
                fatalerror("Can't find the class of the frame  '%s' (%s)\n", newframe->name, consttmpname);
                calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                return 0;
            }
            calceph_snprintf(consttmpname, 512, "FRAME_%d_CLASS_ID", newframe->frame_id);
            if (calceph_txtpck_getconstant_int(&res->filedata.txtfk.txtpckfile, consttmpname, &newframe->class_id) == 0)
            {
                fatalerror("Can't find the class_id of the frame  '%s' (%s)\n", newframe->name, consttmpname);
                calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                return 0;
            }
            calceph_snprintf(consttmpname, 512, "FRAME_%d_CENTER", newframe->frame_id);
            if (calceph_txtpck_getconstant_int(&res->filedata.txtfk.txtpckfile, consttmpname, &newframe->center) == 0)
            {
                fatalerror("Can't find the center of the frame  '%s' (%s)\n", newframe->name, consttmpname);
                calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                return 0;
            }

            calceph_snprintf(consttmpname, 512, "TKFRAME_%d_RELATIVE", newframe->frame_id);
            if (calceph_txtpck_getconstant_txtpckvalue(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                       &newframe->tkframe_relative))
            {
                /* check if relative to J2000 frame */
                if (calceph_txtpck_cmpszvalue(newframe->tkframe_relative, "'J2000'") == 0)
                    newframe->tkframe_relative_id = 1;

                calceph_snprintf(consttmpname, 512, "TKFRAME_%d_SPEC", newframe->frame_id);
                if (calceph_txtpck_getconstant_txtpckvalue(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                           &newframe->tkframe_spec) == 0)
                {
                    fatalerror("Can't find the tkframe spec of the frame  '%s' (%s)\n", newframe->name, consttmpname);
                    calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                    return 0;
                }
                if (calceph_txtpck_cmpszvalue(newframe->tkframe_spec, "'MATRIX'") == 0)
                {
                    calceph_snprintf(consttmpname, 512, "TKFRAME_%d_MATRIX", newframe->frame_id);
                    if (calceph_txtpck_getconstant_matrix3x3(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                             newframe->tkframe_matrix) == 0)
                    {
                        fatalerror("Can't find or read the tkframe matrix of the frame  '%s' (%s)\n", newframe->name,
                                   consttmpname);
                        calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                        return 0;
                    }
                }
                else if (calceph_txtpck_cmpszvalue(newframe->tkframe_spec, "'ANGLES'") == 0)
                {
                    calceph_snprintf(consttmpname, 512, "TKFRAME_%d_ANGLES", newframe->frame_id);
                    if (calceph_txtpck_getconstant_matrix1x3(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                             newframe->tkframe_angles) == 0)
                    {
                        fatalerror("Can't find or read the tkframe angles of the frame  '%s' (%s)\n", newframe->name,
                                   consttmpname);
                        calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                        return 0;
                    }
                    calceph_snprintf(consttmpname, 512, "TKFRAME_%d_AXES", newframe->frame_id);
                    if (calceph_txtpck_getconstant_intmatrix1x3(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                                newframe->tkframe_axes) == 0)
                    {
                        fatalerror("Can't find or read the tkframe angles of the frame  '%s' (%s)\n", newframe->name,
                                   consttmpname);
                        calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                        return 0;
                    }
                    calceph_snprintf(consttmpname, 512, "TKFRAME_%d_UNITS", newframe->frame_id);
                    if (calceph_txtpck_getconstant_txtpckvalue(&res->filedata.txtfk.txtpckfile, consttmpname,
                                                               &newframe->tkframe_unit) == 0)
                    {
                        fatalerror("Can't find or read the units of the tkframe angles of the frame  '%s' (%s)\n",
                                   newframe->name, consttmpname);
                        calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                        return 0;
                    }
                    /* convert to radians the euler angles */
                    if (calceph_txtfk_convert_eulerangles(newframe->tkframe_angles, newframe->tkframe_unit) == 0)
                    {
                        fatalerror("Invalid or unsupported units for the tkframe angles of the frame  '%s' (%s)\n",
                                   newframe->name, consttmpname);
                        calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                        return 0;
                    }
                    /* create the matrix associated to the euler angles */
                    if (calceph_txtfk_creatematrix_eulerangles(newframe->tkframe_matrix, newframe->tkframe_angles,
                                                               newframe->tkframe_axes) == 0)
                    {
                        fatalerror("Invalid axes for the tkframe angles of the frame  '%s' (%s)\n", newframe->name,
                                   consttmpname);
                        calceph_txtpck_close(&res->filedata.txtfk.txtpckfile);
                        return 0;
                    }
                }
            }

            /* go to the next frame */
            listconstant = listconstant->next;
            calceph_snprintf(consttmpname, 512, "FRAME_%d_", newframe->frame_id);
            while (listconstant != NULL && strncmp(listconstant->name, consttmpname, strlen(consttmpname)) == 0)
            {
                listconstant = listconstant->next;
            }
            calceph_snprintf(consttmpname, 512, "TKFRAME_%d_", newframe->frame_id);
            while (listconstant != NULL && strncmp(listconstant->name, consttmpname, strlen(consttmpname)) == 0)
            {
                listconstant = listconstant->next;
            }
        }
        else
        {
            listconstant = listconstant->next;
        }
    }

    fclose(file);
#if DEBUG
    calceph_txtfk_debug(&res->filedata.txtfk);
#endif

    return 1;
}

/*--------------------------------------------------------------------------*/
/*! Close the  text PCK kernel file

  @param eph (inout) descriptor of the ephemeris.
*/
/*--------------------------------------------------------------------------*/
void calceph_txtfk_close(struct TXTFKfile *eph)
{
    struct TXTFKframe *listframe = eph->listframe;

    calceph_txtpck_close(&eph->txtpckfile);

    while (listframe != NULL)
    {
        struct TXTFKframe *pframe = listframe;

        listframe = listframe->next;
        free(pframe);
    }
}

/*--------------------------------------------------------------------------*/
/*! return the frame associated to the specified value from this kernel
    return NULL if not found.
  @param eph (in) descriptor of the ephemeris.
  @param name (in) value to find
*/
/*--------------------------------------------------------------------------*/
const struct TXTFKframe *calceph_txtfk_findframe2(const struct TXTFKfile *eph, const struct TXTPCKvalue *name)
{
    const struct TXTFKframe *listframe = eph->listframe;

    const struct TXTFKframe *frame = NULL;

    while (listframe != NULL && frame == NULL)
    {
        if (calceph_txtpck_cmpvalue(listframe->frame_name, name) == 0)
            frame = listframe;
        listframe = listframe->next;
    }
    return frame;
}

/*--------------------------------------------------------------------------*/
/*! return the frame associated to the specified value from this kernel
    return NULL if not found.
  @param eph (in) descriptor of the ephemeris.
  @param cst (in) constant to find
*/
/*--------------------------------------------------------------------------*/
const struct TXTFKframe *calceph_txtfk_findframe(const struct TXTFKfile *eph, const struct TXTPCKconstant *cst)
{
    const struct TXTFKframe *listframe = eph->listframe;

    const struct TXTFKframe *frame;

    int found, id;

    frame = calceph_txtfk_findframe2(eph, cst->value);

    /* try with value of the constant */
    found = calceph_txtpck_getconstant_int2(cst, &id);
    if (frame == NULL && found == 1)
    {
        listframe = eph->listframe;
        while (listframe != NULL && frame == NULL)
        {
            if (listframe->frame_id == id)
                frame = listframe;
            listframe = listframe->next;
        }
    }

    return frame;
}

/*--------------------------------------------------------------------------*/
/*! return the frame associated to the specified value from this kernel
    return NULL if not found.
  @param eph (in) descriptor of the ephemeris.
  @param id (in) value to find
*/
/*--------------------------------------------------------------------------*/
const struct TXTFKframe *calceph_txtfk_findframe_id(const struct TXTFKfile *eph, int id)
{
    struct TXTFKframe *listframe = eph->listframe;

    struct TXTFKframe *frame = NULL;

    while (listframe != NULL && frame == NULL)
    {
        if (listframe->frame_id == id)
            frame = listframe;
        listframe = listframe->next;
    }
    return frame;
}

/*--------------------------------------------------------------------------*/
/*! convert the angles in radians according to the provided units
  return 0 if the units are invalid
  return 1 on success
  @param tkframe_angles (inout) array of 3 angles.
  @param tkframe_unit (in) units of the angles
*/
/*--------------------------------------------------------------------------*/
static int calceph_txtfk_convert_eulerangles(double tkframe_angles[3], struct TXTPCKvalue *tkframe_unit)
{
    double atorad = 0.E0;

    int k;

    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'DEGREES'") == 0)
    {
        atorad = M_PI / 180.E0;
    }
    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'RADIANS'") == 0)
    {
        atorad = 1.E0;
    }
    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'ARCSECONDS'") == 0)
    {
        atorad = M_PI / (180.E0 * 3600E0);
    }
    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'ARCMINUTES'") == 0)
    {
        atorad = M_PI / (180.E0 * 60E0);
    }
    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'HOURANGLE'") == 0)
    {
        atorad = M_PI / 12.E0;
    }
    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'MINUTEANGLE'") == 0)
    {
        atorad = M_PI / (12.E0 * 60E0);
    }
    if (calceph_txtpck_cmpszvalue(tkframe_unit, "'SECONDANGLE'") == 0)
    {
        atorad = M_PI / (12.E0 * 3600.E0);
    }
    if (atorad == 0.E0)
        return 0;

    for (k = 0; k < 3; k++)
        tkframe_angles[k] *= atorad;
    return 1;
}

/*--------------------------------------------------------------------------*/
/*! create the rotation matrix of angle A around the axis 1
  @param matrix (out) rotation matrix (3x3)
  @param A (in) angles.
*/
/*--------------------------------------------------------------------------*/
void calceph_txtfk_creatematrix_axes1(double matrix[3][3], const double A)
{
    matrix[0][0] = 1.E0;
    matrix[0][1] = 0.E0;
    matrix[0][2] = 0.E0;
    matrix[1][0] = 0.E0;
    matrix[1][1] = cos(A);
    matrix[1][2] = sin(A);
    matrix[2][0] = 0.E0;
    matrix[2][1] = -sin(A);
    matrix[2][2] = cos(A);
}

/*--------------------------------------------------------------------------*/
/*! create the rotation matrix of angle A around the axis 2
  @param matrix (out) rotation matrix (3x3)
  @param A (in) angles.
*/
/*--------------------------------------------------------------------------*/
static void calceph_txtfk_creatematrix_axes2(double matrix[3][3], const double A)
{
    matrix[0][0] = cos(A);
    matrix[0][1] = 0.E0;
    matrix[0][2] = -sin(A);
    matrix[1][0] = 0.E0;
    matrix[1][1] = 1.E0;
    matrix[1][2] = 0.E0;
    matrix[2][0] = sin(A);
    matrix[2][1] = 0.E0;
    matrix[2][2] = cos(A);
}

/*--------------------------------------------------------------------------*/
/*! create the rotation matrix of angle A around the axis 3
  @param matrix (out) rotation matrix (3x3)
  @param A (in) angles.
*/
/*--------------------------------------------------------------------------*/
static void calceph_txtfk_creatematrix_axes3(double matrix[3][3], const double A)
{
    matrix[0][0] = cos(A);
    matrix[0][1] = sin(A);
    matrix[0][2] = 0.E0;
    matrix[1][0] = -sin(A);
    matrix[1][1] = cos(A);
    matrix[1][2] = 0.E0;
    matrix[2][0] = 0.E0;
    matrix[2][1] = 0.E0;
    matrix[2][2] = 1.E0;
}

/*--------------------------------------------------------------------------*/
/*! create the rotation matrix of angle A around the axis k
  return 0 if k is invalid
  return 1 on sucess
  @param matrix (out) rotation matrix (3x3)
  @param k (in) axis number (from 1 to 3)
  @param A (in) angles.
*/
/*--------------------------------------------------------------------------*/
static int calceph_txtfk_creatematrix_axesk(double matrix[3][3], int k, const double A)
{
    switch (k)
    {
        case 1:
            calceph_txtfk_creatematrix_axes1(matrix, A);
            break;
        case 2:
            calceph_txtfk_creatematrix_axes2(matrix, A);
            break;
        case 3:
            calceph_txtfk_creatematrix_axes3(matrix, A);
            break;
        default:
            return 0;
    }

    return 1;
}

/*--------------------------------------------------------------------------*/
/*! compute M = A * B
  @param M (out) A * B
  @param A (in) 1st matrix
  @param B (in) 2nd matrix
*/
/*--------------------------------------------------------------------------*/
void calceph_matrix3x3_prod(double M[3][3], double A[3][3], double B[3][3])
{
    int j, k;

    for (j = 0; j < 3; j++)
    {
        for (k = 0; k < 3; k++)
        {
            M[j][k] = A[j][0] * B[0][k] + A[j][1] * B[1][k] + A[j][2] * B[2][k];
        }
    }
}

/*--------------------------------------------------------------------------*/
/*! create the rotation matrix associated to the angles in radians and the axes
  return 1 on success
  return 0 if the axes are invalid
  @param tkframe_matrix (out) rotation matrix (3x3)
  @param tkframe_angles (in) array of 3 angles, expressed in radians.
  @param tkframe_axes (in) axes of the angles (from 1 to 3)
*/
/*--------------------------------------------------------------------------*/
int calceph_txtfk_creatematrix_eulerangles(double tkframe_matrix[9], const double tkframe_angles[3],
                                           int tkframe_axes[3])
{
    int j, k;

    double matrix0[3][3], matrix1[3][3], matrix2[3][3];

    double matrix12[3][3], matrix123[3][3];

    if (calceph_txtfk_creatematrix_axesk(matrix0, tkframe_axes[0], tkframe_angles[0]) == 0)
        return 0;
    if (calceph_txtfk_creatematrix_axesk(matrix1, tkframe_axes[1], tkframe_angles[1]) == 0)
        return 0;
    if (calceph_txtfk_creatematrix_axesk(matrix2, tkframe_axes[2], tkframe_angles[2]) == 0)
        return 0;
    calceph_matrix3x3_prod(matrix12, matrix1, matrix2);
    calceph_matrix3x3_prod(matrix123, matrix0, matrix12);
    for (j = 0; j < 3; j++)
    {
        for (k = 0; k < 3; k++)
        {
            tkframe_matrix[3 * j + k] = matrix123[j][k];
        }
    }
    return 1;
}

/*--------------------------------------------------------------------------*/
/*! create the  angles in radians from the rotation matrix
, for the rotation 3-1-3
  return 1 on success
  return 0 if the axes are invalid
  @param tkframe_matrix (in) rotation matrix (3x3)
  @param tkframe_angles (out) array of 3 angles, expressed in radians.
*/
/*--------------------------------------------------------------------------*/
int calceph_txtfk_createeulerangles_matrix(double tkframe_angles[3], double tkframe_matrix[3][3])
{
    double theta, psi, phi;

    if (fabs(tkframe_matrix[2][0]) == 1.E0)
    {
        psi = 0.E0;
        theta = 0.E0;
        phi = atan2(tkframe_matrix[1][0], tkframe_matrix[1][1]);
    }
    else
    {
        psi = atan2(tkframe_matrix[0][2], tkframe_matrix[1][2]);
        theta = acos(tkframe_matrix[2][2]);
        phi = atan2(tkframe_matrix[2][0], -tkframe_matrix[2][1]);
    }
    tkframe_angles[0] = psi;
    tkframe_angles[1] = theta;
    tkframe_angles[2] = phi;

    return 1;
}
