/*-----------------------------------------------------------------*/
/*! 
  \file calcephbinpckio.c 
  \brief perform I/O on binary PCK KERNEL ephemeris data file
         read all constants from a binary PCK KERNEL Ephemeris data.
         

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

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephspice.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
#if DEBUG
void calceph_binpck_debugsegment(struct SPKSegmentHeader *seg);
#endif
static int calceph_binpck_readlistsegment(FILE * file, const char *filename, struct SPKSegmentList **list, int fwd,
                                          int bwd, enum SPKBinaryFileFormat reqconvert);
static int calceph_binpck_readsegment_header(FILE * file, const char *filename, struct SPKSegmentHeader *seg);

/*--------------------------------------------------------------------------*/
/*!  
     Read the header data block for the binary PCK ephemeris file
     Read the comment block.
     Read the segment block.
     
  @return  1 on sucess and 0 on failure
  
  @param file (inout) file descriptor.   
  @param filename (in) A character string giving the name of an ephemeris data file.   
  @param res (out) descriptor of the ephemeris.  
*/
/*--------------------------------------------------------------------------*/
int calceph_binpck_open(FILE * file, const char *filename, struct SPICEkernel *res)
{
    struct SPKHeader header;

    struct SPKSegmentList *listseg;

    buffer_error_t buffer_error;

  /*---------------------------------------------------------*/
    /* initialize with empty values if a failure occurs during the initialization */
  /*---------------------------------------------------------*/
    res->filetype = DAF_PCK;
    res->filedata.spk.list_seg = NULL;
    res->filedata.spk.file = NULL;
    res->filedata.spk.prefetch = 0;
    res->filedata.spk.mmap_buffer = NULL;
    res->filedata.spk.mmap_size = 0;
    res->filedata.spk.mmap_used = 0;

    listseg = NULL;
 /*---------------------------------------------------------*/
    /* read the header block */
 /*---------------------------------------------------------*/
    /* go to the head of the file */
    if (fseeko(file, 0, SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to the beginning of the ephemeris file '%s'\nSystem error : '%s'\n",
                   filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    if (fread(&header, sizeof(header), 1, file) != 1)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read the beginning of the ephemeris file '%s'\nSystem error : '%s'\n",
                   filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }

  /*---------------------------------------------------------*/
    /* check if Binary File Format Identification requires a conversion for this
     * hardware */
  /*---------------------------------------------------------*/
    res->filedata.spk.bff = calceph_bff_detect(&header);

#if DEBUG
    calceph_spk_debugheader(&header);
#endif
 /*---------------------------------------------------------*/
    /* check the ftp string */
 /*---------------------------------------------------------*/
    if (calceph_spk_ftp(header.ftp) == 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("The FTP string is not valid in the file '%s'\n", filename);
        return 0;
        /* GCOVR_EXCL_STOP */
    }
 /*---------------------------------------------------------*/
    /* read the list of segments */
 /*---------------------------------------------------------*/
    if (calceph_binpck_readlistsegment(file, filename, &listseg, header.fwd, header.bwd, res->filedata.spk.bff) == 0)
    {
        return 0;
    }
    res->filedata.spk.header = header;
    res->filedata.spk.list_seg = listseg;
    res->filedata.spk.file = file;

    return 1;
}

/*--------------------------------------------------------------------------*/
/*! Close the SPICE ephemeris file.
     
  @return  1 on sucess and 0 on failure
  
  @param eph (inout) descriptor of the ephemeris.  
*/
/*--------------------------------------------------------------------------*/
void calceph_binpck_close(struct SPKfile *eph)
{
    calceph_spk_close(eph);
}

/*--------------------------------------------------------------------------*/
/*! read the list of segments in the file : only the header                 

   @param file (inout) file descriptor.   
   @param filename (in) A character string giving the name of an ephemeris data file.   
   @param list (inout) pointer to the list 
   @param fwd (in) record number to read
   @param bwd (in) previous record number
   @param reqconvert (in) binary file format
*/
/*--------------------------------------------------------------------------*/
static int calceph_binpck_readlistsegment(FILE * file, const char *filename, struct SPKSegmentList **list, int fwd,
                                          int PARAMETER_UNUSED(bwd), enum SPKBinaryFileFormat reqconvert)
{
#if HAVE_PRAGMA_UNUSED
#pragma unused(bwd)
#endif
    char descriptor[DAF_RECORD_LEN];

    double *ddescriptor = (double *) descriptor;

    char segmentid[DAF_RECORD_LEN];

    int next = 0, count;

    buffer_error_t buffer_error;

#if DEBUG
    int prev;
#endif
    int res = 1;

    int j, offset;

    off_t ofwd = fwd;           /* stored in a large number for large file */

    struct SPKSegmentList *pnew;

#if DEBUG
    printf("calceph_binpck_readlistsegment(%p,%d)\n", *list, fwd);
#endif

    /* go to the head of the file */
    if (fseeko(file, DAF_RECORD_LEN * (ofwd - 1), SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror
            ("Can't jump to the segment descriptor record at location %d of the ephemeris file '%s'\nSystem error : '%s'\n",
             fwd, filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    if (fread(descriptor, sizeof(char), DAF_RECORD_LEN, file) != DAF_RECORD_LEN)
    {
        /* GCOVR_EXCL_START */
        fatalerror
            ("Can't read the segment descriptor record at location  %d of the ephemeris file '%s'\nSystem error : '%s'\n",
             fwd, filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    calceph_bff_convert_array_double(ddescriptor, DAF_RECORD_LEN / 8, reqconvert);

    next = (int) ddescriptor[0];
#if DEBUG
    prev = (int) ddescriptor[1];
#endif
    count = (int) ddescriptor[2];

#if DEBUG
    printf("iprev=%d %d %d\n", prev, next, count);

    for (j = 0; j < DAF_RECORD_LEN / 8; j++)
        printf("%g ", ((double *) descriptor)[j]);
    printf("\n");
#endif

    if (count > 0)
    {

        if (fread(segmentid, sizeof(char), DAF_RECORD_LEN, file) != DAF_RECORD_LEN)
        {
            /* GCOVR_EXCL_START */
            fatalerror
                ("Can't read the segment id record at location  %d of the ephemeris file '%s'\nSystem error : '%s'\n",
                 fwd + 1, filename, calceph_strerror_errno(buffer_error));
            /* GCOVR_EXCL_STOP */
            return 0;
        }

#if DEBUG
        for (j = 0; j < DAF_RECORD_LEN; j++)
            printf("%c ", segmentid[j]);
        printf("\n");
#endif

        /* create a new element of the list */
        pnew = (struct SPKSegmentList *) malloc(sizeof(struct SPKSegmentList));
        if (pnew == NULL)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't allocate memory for a segment descritor\n");
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        /* insert in the list */
        pnew->next = *list;
        pnew->prev = NULL;
        if (*list != NULL)
            (*list)->prev = pnew;
        *list = pnew;
        pnew->recordnumber = fwd;
        pnew->array_seg.count = count;

        /* fill the content of the list */
        offset = 2;
        for (j = 0; j < count && res == 1; j++)
        {
            int *parint = (int *) (ddescriptor + offset + 3);

            struct SPKSegmentHeader *seg = pnew->array_seg.array + j;

            calceph_bff_reorder_array_int(parint, 5, reqconvert);

            seg->T_begin = ddescriptor[offset + 1];
            seg->T_end = ddescriptor[offset + 2];
            seg->body = parint[0];
            seg->center = -1;
            seg->refframe = parint[1];
            seg->datatype = (enum SPKdatatype) parint[2];
            seg->rec_begin = parint[3];
            seg->rec_end = parint[4];
            seg->bff = reqconvert;

            memcpy(seg->id, segmentid + SEGMENTID_LEN * j, SEGMENTID_LEN);
            memcpy(seg->descriptor, ddescriptor + offset, NS_SPK * 8);
#if DEBUG
            printf("before calceph_binpck_readsegment_header:\n");
            calceph_binpck_debugsegment(seg);
#endif

            res = calceph_binpck_readsegment_header(file, filename, seg);
#if DEBUG
            printf("after calceph_binpck_readlistsegment:\n");
            calceph_binpck_debugsegment(seg);
#endif
            offset += NS_SPK;
        }
    }

    /* go the next records */
    if (next != 0 && res != 0)
    {
        res = calceph_binpck_readlistsegment(file, filename, list, next, fwd, reqconvert);
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! read the data header of this segmentid
   @param file (inout) file descriptor.   
   @param filename (in) A character string giving the name of an ephemeris data file.   
   @param seg (inout) segment 
*/
/*--------------------------------------------------------------------------*/
static int calceph_binpck_readsegment_header(FILE * file, const char *filename, struct SPKSegmentHeader *seg)
{
    double drecord[12];

    int res = 1;

    switch (seg->datatype)
    {
        case SPK_SEGTYPE103:
        case SPK_SEGTYPE102:
        case SPK_SEGTYPE3:
        case SPK_SEGTYPE2:
            res = calceph_spk_readword(file, filename, seg->rec_end - 3, seg->rec_end, drecord);
            if (res == 1)
            {
                calceph_bff_convert_array_double(drecord, 4, seg->bff);
                seg->seginfo.data2.T_begin = drecord[0];
                seg->seginfo.data2.T_len = drecord[1];
                seg->seginfo.data2.count_dataperrecord = (int) drecord[2];
                seg->seginfo.data2.count_record = (int) drecord[3];
            }
            break;

        case SPK_SEGTYPE120:
        case SPK_SEGTYPE20:
            res = calceph_spk_readword(file, filename, seg->rec_end - 6, seg->rec_end, drecord);
            if (res == 1)
            {
                calceph_bff_convert_array_double(drecord, 7, seg->bff);
                seg->seginfo.data20.dscale = drecord[0];
                seg->seginfo.data20.tscale = drecord[1];
                seg->seginfo.data20.T_init_JD = drecord[2];
                seg->seginfo.data20.T_init_FRACTION = drecord[3];
                seg->seginfo.data20.T_begin = seg->seginfo.data20.T_init_JD + seg->seginfo.data20.T_init_FRACTION;
                seg->seginfo.data20.T_len = drecord[4] * seg->seginfo.data20.tscale;
                seg->seginfo.data20.count_dataperrecord = (int) drecord[5];
                seg->seginfo.data20.count_record = (int) drecord[6];
            }
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unsupported segment (type=%d).", (int) seg->datatype);
            res = 0;
            /* GCOVR_EXCL_STOP */
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! debug the segment 
   @param seg (inout) segment 
*/
/*--------------------------------------------------------------------------*/
#if DEBUG
void calceph_binpck_debugsegment(struct SPKSegmentHeader *seg)
{
    printf("segment ID : '%.*s'\n", SEGMENTID_LEN, seg->id);
    printf("T_begin : %e -> %e\n", seg->T_begin, seg->T_end);
    printf("body : %d \n", seg->body);
    printf("refframe : %d \n", seg->refframe);
    printf("datatype : %d \n", (int) seg->datatype);
    printf("rec_begin : %d \n", seg->rec_begin);
    printf("rec_end : %d \n", seg->rec_end);
    switch (seg->datatype)
    {
        case SPK_SEGTYPE103:
        case SPK_SEGTYPE102:
        case SPK_SEGTYPE3:
        case SPK_SEGTYPE2:
            printf("information of the segment : %e %e %d %d\n",
                   seg->seginfo.data2.T_begin, seg->seginfo.data2.T_len,
                   seg->seginfo.data2.count_dataperrecord, seg->seginfo.data2.count_record);
            break;

        case SPK_SEGTYPE120:
        case SPK_SEGTYPE20:
            printf("information of the segment : %e %e %d %d\n",
                   seg->seginfo.data20.T_begin, seg->seginfo.data20.T_len,
                   seg->seginfo.data20.count_dataperrecord, seg->seginfo.data20.count_record);
            printf("additional information of the segment : %e %e %e %e\n",
                   seg->seginfo.data20.T_init_JD, seg->seginfo.data20.T_init_FRACTION,
                   seg->seginfo.data20.dscale, seg->seginfo.data20.tscale);
            break;

        default:
            printf("Unsupported segment (type=%d).", (int) seg->datatype);
            break;
    }
}
#endif
