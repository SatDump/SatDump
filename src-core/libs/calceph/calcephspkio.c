/*-----------------------------------------------------------------*/
/*!
  \file calcephspkio.c
  \brief perform I/O on SPICE KERNEL ephemeris data file
         interpolate SPICE KERNEL Ephemeris data.
         compute position and velocity from SPICE KERNEL ephemeris file.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, LTE, CNRS, Observatoire de
  Paris.

   Copyright, 2011-2025, CNRS
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
knowledge of the CeCILL-C,CeCILL-B or CeCILL license and that you accept its
terms.
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
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/*--------------------------------------------------------------------------*/
/* private functions */
/*--------------------------------------------------------------------------*/
#if DEBUG
static void calceph_spk_debugsegment(struct SPKSegmentHeader *seg);
#endif
static int calceph_spk_readlistsegment(FILE * file, const char *filename, struct SPKSegmentList **list,
                                       int fwd, int bwd, enum SPKBinaryFileFormat reqconvert);
static int calceph_spk_readsegment_header(FILE * file, const char *filename, struct SPKSegmentHeader *seg);

static void calceph_spk_convertheader(struct SPKHeader *header, enum SPKBinaryFileFormat reqconvert);

static int calceph_bff_convert_int(int x, enum SPKBinaryFileFormat reqconvert);

/*--------------------------------------------------------------------------*/
/*!
     Read the header data block for the SPK ephemeris file
     Read the comment block.
     Read the segment block.

  @return  1 on sucess and 0 on failure

  @param file (inout) file descriptor.
  @param filename (in) A character string giving the name of an ephemeris data
  file.
  @param res (out) descriptor of the ephemeris.
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_open(FILE *file, const char *filename, struct SPICEkernel *res)
{
    buffer_error_t buffer_error;
    struct SPKHeader header;

    struct SPKSegmentList *listseg;

    listseg = NULL;

  /*---------------------------------------------------------*/
    /* initialize with empty values if a failure occurs during the initialization */
  /*---------------------------------------------------------*/
    res->filetype = DAF_SPK;
    res->filedata.spk.list_seg = NULL;
    res->filedata.spk.file = NULL;
    res->filedata.spk.prefetch = 0;
    res->filedata.spk.mmap_buffer = NULL;
    res->filedata.spk.mmap_size = 0;
    res->filedata.spk.mmap_used = 0;

  /*---------------------------------------------------------*/
    /* read the header block */
  /*---------------------------------------------------------*/
    /* go to the head of the file */
    if (fseeko(file, 0, SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to the beginning of the ephemeris file '%s'\nSystem "
                   "error : '%s'\n", filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    if (fread(&header, sizeof(header), 1, file) != 1)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read the beginning of the ephemeris file '%s'\nSystem "
                   "error : '%s'\n", filename, calceph_strerror_errno(buffer_error));
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
    if (calceph_spk_readlistsegment(file, filename, &listseg, header.fwd, header.bwd, res->filedata.spk.bff) == 0)
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
void calceph_spk_close(struct SPKfile *eph)
{
    while (eph->list_seg != NULL)
    {
        struct SPKSegmentList *listseg = eph->list_seg;

        /* free the header */
        int iheader;

        for (iheader = 0; iheader < listseg->array_seg.count; iheader++)
        {
            struct SPKSegmentHeader *pheader = listseg->array_seg.array + iheader;

            switch (pheader->datatype)
            {
                case SPK_SEGTYPE1:
                    free(pheader->seginfo.data1.directory);
                    break;
                case SPK_SEGTYPE5:
                    free(pheader->seginfo.data5.directory);
                    break;
                case SPK_SEGTYPE9:
                case SPK_SEGTYPE13:
                    free(pheader->seginfo.data13.directory);
                    break;
                case SPK_SEGTYPE18:
                    free(pheader->seginfo.data18.directory);
                    break;
                case SPK_SEGTYPE19:
                    if (pheader->seginfo.data19.boundary_times_directory)
                        free(pheader->seginfo.data19.boundary_times_directory);
                    free(pheader->seginfo.data19.boundary_times);
                    free(pheader->seginfo.data19.mini_segments);
                    break;
                case SPK_SEGTYPE21:
                    free(pheader->seginfo.data21.directory);
                    break;
                default:
                    break;
            }
        }

        /* free the current element and go tothe next segment */
        eph->list_seg = eph->list_seg->next;
        free(listseg);
    }
    if (eph->file)
        fclose(eph->file);
    if (eph->mmap_buffer)
    {
#if HAVE_MMAP
        if (eph->mmap_used == 1)
        {
            munmap(eph->mmap_buffer, eph->mmap_size);
        }
        else
#endif
        {
            free(eph->mmap_buffer);
        }
    }
}

/*--------------------------------------------------------------------------*/
/*! Prefetch the SPICE ephemeris file.

  @return  1 on sucess and 0 on failure

  @param pspk (inout) descriptor of the ephemeris.
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_prefetch(struct SPKfile *pspk)
{
    int res = 1;

    off_t newlen;

    if (pspk->prefetch == 0 && pspk->bff == BFF_NATIVE_IEEE)
    {
        if (fseeko(pspk->file, 0, SEEK_END) != 0)
        {
            return 0;
        }
        newlen = ftello(pspk->file);
        if (newlen == -1)
        {
            return 0;
        }

        if (fseeko(pspk->file, 0, SEEK_SET) != 0)
        {
            return 0;
        }

#if HAVE_MMAP
#ifdef MAP_POPULATE
        pspk->mmap_buffer = (double *) mmap(NULL, (size_t) newlen, PROT_READ, MAP_PRIVATE | MAP_POPULATE,
                                            fileno(pspk->file), 0);
#else
        pspk->mmap_buffer = (double *) mmap(NULL, (size_t) newlen, PROT_READ, MAP_PRIVATE, fileno(pspk->file), 0);
#endif
        if (pspk->mmap_buffer == MAP_FAILED)
        {
            pspk->mmap_buffer = NULL;
            return 0;
        }
        pspk->mmap_size = (size_t) newlen;
        pspk->mmap_used = 1;
#else
        if (pspk->mmap_size < (unsigned long) newlen)
        {
            pspk->mmap_buffer = (double *) realloc(pspk->mmap_buffer, newlen);
            if (pspk->mmap_buffer == NULL)
            {
                return 0;
            }
            pspk->mmap_size = (unsigned long) newlen;
            pspk->mmap_used = 0;
        }

        if (fread(pspk->mmap_buffer, sizeof(char), newlen, pspk->file) != (size_t) newlen)
        {
            free(pspk->mmap_buffer);
            pspk->mmap_buffer = NULL;
            pspk->mmap_size = 0;
            return 0;
        }
#endif
        pspk->prefetch = 1;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*!
     convert the numbers of the header of a SPK file to the native number

  @param header (inout) header of SPK
  @param reqconvert (in) type of data in the file
*/
/*--------------------------------------------------------------------------*/
static void calceph_spk_convertheader(struct SPKHeader *header, enum SPKBinaryFileFormat reqconvert)
{
    header->nd = calceph_bff_convert_int(header->nd, reqconvert);
    header->ni = calceph_bff_convert_int(header->ni, reqconvert);
    header->fwd = calceph_bff_convert_int(header->fwd, reqconvert);
    header->bwd = calceph_bff_convert_int(header->bwd, reqconvert);
    header->free = calceph_bff_convert_int(header->free, reqconvert);
}

/*--------------------------------------------------------------------------*/
/*!
    allocate count_directory directories
    and store the address of allocated memory to directory

    return 1 on success
    return 0 on error

  @param directory (out) pointer to 'count_directory' floating-point numbers
  @param count_directory (in) number of directory to allocate


*/
/*--------------------------------------------------------------------------*/
int calceph_spk_allocate_directory(int count_directory, double **directory)
{
    *directory = (double *) malloc(count_directory * sizeof(double));
    if (*directory == NULL)
    {
        fatalerror("Can't allocate memory for a segment directory with %d elements\n", count_directory);
        return 0;
    }

    return 1;
}

#if DEBUG
/*--------------------------------------------------------------------------*/
/*!
     Debug the header of a SPK file

  @param header (in) header of SPK
*/
/*--------------------------------------------------------------------------*/
void calceph_spk_debugheader(const struct SPKHeader *header)
{
    int k;

    printf("file architecture and type : '%*s'\n", IDWORD_LEN, header->idword);
    printf("ND : %d\n", header->nd);
    printf("NI : %d\n", header->ni);
    printf("Internal file name : '%*s'\n", IFNAME_LEN, header->ifname);
    printf("FWD : %d\n", header->fwd);
    printf("BWD : %d\n", header->bwd);
    printf("FREE : %d\n", header->free);
    printf("Binary file format ID : '%*s'\n", BFF_LEN, header->bff);
    printf("FTP corruption test string : \n");
    for (k = 0; k < FTP_LEN; k++)
        printf("%d ", (int) header->ftp[k]);
    printf("\n");
}
#endif

/*--------------------------------------------------------------------------*/
/*! return 0 if fptbuf is not equal to the string
  FTPSTR:<13>:<10>:<13><10>:<13><0>:<129>:<16><206>:ENDFTP

  return 1 if ftp is esqual to this string

  @param ftpbuf (in) buffer to checked
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_ftp(const unsigned char ftpbuf[FTP_LEN])
{
    const unsigned char validftp[FTP_LEN] = { 'F', 'T', 'P', 'S', 'T', 'R', ':', 13, ':', 10, ':', 13, 10, ':',
        13, 0, ':', 129, ':', 16, 206, ':', 'E', 'N', 'D', 'F', 'T', 'P'
    };
    int j;

    int res = 1;

    for (j = 0; j < FTP_LEN; j++)
    {
        if (ftpbuf[j] != validftp[j])
            res = 0;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*! read the list of segments in the file : only the header

   @param file (inout) file descriptor.
   @param filename (in) A character string giving the name of an ephemeris data
   file.
   @param list (inout) pointer to the list
   @param fwd (in) record number to read
   @param bwd (in) previous record number
   @param reqconvert (in) binary file format
*/
/*--------------------------------------------------------------------------*/
static int calceph_spk_readlistsegment(FILE *file, const char *filename, struct SPKSegmentList **list,
                                       int fwd, int PARAMETER_UNUSED(bwd), enum SPKBinaryFileFormat reqconvert)
{
#if HAVE_PRAGMA_UNUSED
#pragma unused(bwd)
#endif
    int next = 0;

    int res = 1;

    buffer_error_t buffer_error;

    do
    {
        char descriptor[DAF_RECORD_LEN];

        double *ddescriptor = (double *) descriptor;

        char segmentid[DAF_RECORD_LEN];

        int count;

#if DEBUG
        int prev;
#endif

        int j, offset;

        struct SPKSegmentList *pnew;

        off_t ofwd = fwd;       /* stored in a large number for large file */

#if DEBUG
        printf("calceph_spk_readlistsegment(%p,%d)\n", *list, fwd);
#endif

        /* go to the head of the file */
        if (fseeko(file, DAF_RECORD_LEN * (ofwd - 1), SEEK_SET) != 0)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't jump to the segment descriptor record at location %d of "
                       "the ephemeris file '%s'\nSystem error : '%s'\n",
                       fwd, filename, calceph_strerror_errno(buffer_error));
            return 0;
            /* GCOVR_EXCL_STOP */
        }
        if (fread(descriptor, sizeof(char), DAF_RECORD_LEN, file) != DAF_RECORD_LEN)
        {
            /* GCOVR_EXCL_START */
            fatalerror("Can't read the segment descriptor record at location  %d of "
                       "the ephemeris file '%s'\nSystem error : '%s'\n",
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
                fatalerror("Can't read the segment id record at location  %d of the "
                           "ephemeris file '%s'\nSystem error : '%s'\n",
                           fwd + 1, filename, calceph_strerror_errno(buffer_error));
                return 0;
                /* GCOVR_EXCL_STOP */
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
                fatalerror("Can't allocate memory for a segment descriptor\n");
                return 0;
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

                calceph_bff_reorder_array_int(parint, 6, reqconvert);

                seg->T_begin = ddescriptor[offset + 1];
                seg->T_end = ddescriptor[offset + 2];
                seg->body = parint[0];
                seg->center = parint[1];
                seg->refframe = parint[2];
                seg->datatype = (enum SPKdatatype) parint[3];
                seg->rec_begin = parint[4];
                seg->rec_end = parint[5];
                seg->bff = reqconvert;

                memcpy(seg->id, segmentid + SEGMENTID_LEN * j, SEGMENTID_LEN);
                memcpy(seg->descriptor, ddescriptor + offset, NS_SPK * 8);
#if DEBUG
                printf("before calceph_spk_readsegment_header:\n");
                calceph_spk_debugsegment(seg);
#endif

                res = calceph_spk_readsegment_header(file, filename, seg);
#if DEBUG
                printf("after calceph_spk_readsegment_header:\n");
                calceph_spk_debugsegment(seg);
#endif
                offset += NS_SPK;
            }
        }

        /* go the next records */
        fwd = next;
    }
    while (next != 0 && res != 0);

    return res;
}

/*--------------------------------------------------------------------------*/
/*! read the double-precision numbers from rec_begin to rec_end (in number of
   double-precision numbers)
   @param file (inout) file descriptor.
   @param filename (in) A character string giving the name of an ephemeris data
   file.
   @param rec_begin (inout) first word to read
   @param rec_end (inout) last word to read
   @param record (out) array to store the words
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_readword(FILE *file, const char *filename, int rec_begin, int rec_end, double *record)
{
    buffer_error_t buffer_error;
    size_t n = (size_t) (rec_end - rec_begin + 1);

    if (fseeko(file, (rec_begin - 1) * sizeof(double), SEEK_SET) != 0)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't jump to the location %lu of the ephemeris file "
                   "'%s'\nSystem error : '%s'\n", rec_begin, filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    if (fread(record, sizeof(double), n, file) != n)
    {
        /* GCOVR_EXCL_START */
        fatalerror("Can't read %d data at location  %d of the ephemeris file "
                   "'%s'\nSystem error : '%s'\n", n, rec_begin, filename, calceph_strerror_errno(buffer_error));
        return 0;
        /* GCOVR_EXCL_STOP */
    }

    return 1;
}

/*--------------------------------------------------------------------------*/
/*! prefetch the data for the segment.
   read the double-precision numbers from rec_begin to rec_end (in number of
   double-precision numbers)

   return 0 if error occurs
   return 1 if no error

   @param pspk (inout) spk kernel.
    pspk is not updated if the data are already prefetched.
   @param seg (in) segment
   @param cache (inout) cache of the object
   @param filename (in) A character string giving the name of an ephemeris data
   file.
   @param rec_begin (in) first word to read
   @param rec_end (in) last word to read
   @param record (out) array to store the words
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_fastreadword(struct SPKfile *pspk, const struct SPKSegmentHeader *seg,
                             struct SPICEcache *cache, const char *filename, int rec_begin, int rec_end,
                             const double **record)
{
    if (pspk->prefetch == 0)
    {
        if (cache->segment != seg || cache->rec_begin != rec_begin)
        {
            size_t newlen = (rec_end - rec_begin + 1) * sizeof(double);

            if (cache->buffer_size < newlen)
            {
                cache->buffer = (double *) realloc(cache->buffer, newlen);
                if (cache->buffer == NULL)
                {
                    /* GCOVR_EXCL_START */
                    fatalerror("Can't allocate memory for %lu bytes.\n", (unsigned long) newlen);
                    return 0;
                    /* GCOVR_EXCL_STOP */
                }
                cache->buffer_size = newlen;
            }
            if (calceph_spk_readword(pspk->file, filename, rec_begin, rec_end, cache->buffer) == 0)
            {
                return 0;
            }
            calceph_bff_convert_array_double(cache->buffer, (rec_end - rec_begin + 1), seg->bff);
            cache->rec_begin = rec_begin;
            cache->rec_end = rec_end;
            cache->segment = seg;
        }
        *record = cache->buffer;
    }
    else
    {                           /* prefect was done before */
        *record = pspk->mmap_buffer + rec_begin - 1;
    }
    return 1;
}

#if DEBUG
/*--------------------------------------------------------------------------*/
/*! debug the segment
   @param seg (inout) segment
*/
/*--------------------------------------------------------------------------*/
void calceph_spk_debugsegment(struct SPKSegmentHeader *seg)
{
    printf("segment ID : '%.*s'\n", SEGMENTID_LEN, seg->id);
    printf("T_begin : %e -> %e\n", seg->T_begin, seg->T_end);
    printf("body : %d \n", seg->body);
    printf("center : %d \n", seg->center);
    printf("refframe : %d \n", seg->refframe);
    printf("datatype : %d \n", (int) seg->datatype);
    printf("rec_begin : %d \n", seg->rec_begin);
    printf("rec_end : %d \n", seg->rec_end);
    switch (seg->datatype)
    {
        case SPK_SEGTYPE120:
        case SPK_SEGTYPE103:
        case SPK_SEGTYPE102:
        case SPK_SEGTYPE20:
        case SPK_SEGTYPE3:
        case SPK_SEGTYPE2:
            printf("information of the segment : %e %e %d %d\n", seg->seginfo.data2.T_begin,
                   seg->seginfo.data2.T_len, seg->seginfo.data2.count_dataperrecord, seg->seginfo.data2.count_record);
            break;

        case SPK_SEGTYPE1:
            printf("information of the segment : %d\n", seg->seginfo.data1.count_record);
            break;

        case SPK_SEGTYPE5:
            printf("information of the segment : %d GM: %g\n", seg->seginfo.data5.count_record, seg->seginfo.data5.GM);
            break;

        case SPK_SEGTYPE17:
            printf("information of the segment : \n");
            printf("Equinoctial Elements : %g %g %g %g %g %g %g %g %g %g %g %g  TDB\n",
                   seg->seginfo.data17.epoch, seg->seginfo.data17.a, seg->seginfo.data17.h,
                   seg->seginfo.data17.k, seg->seginfo.data17.mean_longitude, seg->seginfo.data17.p,
                   seg->seginfo.data17.q, seg->seginfo.data17.rate_longitude_periapse,
                   seg->seginfo.data17.mean_longitude_rate, seg->seginfo.data17.longitude_ascending_node_rate,
                   seg->seginfo.data17.ra_pole, seg->seginfo.data17.de_pole);
            break;

        case SPK_SEGTYPE18:
            printf("information of the segment : "
                   "subtype %d - degree %d - Window's size %d - TDB\n",
                   seg->seginfo.data18.subtype, seg->seginfo.data18.degree, seg->seginfo.data18.window_size);
            break;

        case SPK_SEGTYPE19:
            printf("information of the segment : %d "
                   "boundary %d - TDB\n",
                   seg->seginfo.data19.number_intervals, seg->seginfo.data19.boundary_choice_flag);
            break;

        case SPK_SEGTYPE13:
            printf("information of the segment : \n");
            break;

        case SPK_SEGTYPE14:
            printf("information of the segment : chebyshev degree :%d\n", seg->seginfo.data14.degree);
            printf("base address of the directory of epochs: %d\n", seg->seginfo.data14.rec_directory_epoch);
            printf("number of items in the directory of epochs: %d\n", seg->seginfo.data14.count_directory_epoch);
            printf("base address of the reference epochs: %d\n", seg->seginfo.data14.rec_reference_epochs);
            printf("number of reference epochs: %d\n", seg->seginfo.data14.count_reference_epochs);
            printf("base address of the packets: %d\n", seg->seginfo.data14.rec_packets);
            printf("number of packets: %d\n", seg->seginfo.data14.count_packets);
            printf("size of the packets : %d\n", seg->seginfo.data14.size_packet);
            printf("offset of the packet data from the start of a packet record: %d\n",
                   seg->seginfo.data14.offset_start_packet);
            break;

        case SPK_SEGTYPE21:
            printf("information of the segment : %d %d\n", seg->seginfo.data21.count_record,
                   seg->seginfo.data21.dlsize);
            break;

        default:
            printf("Unsupported segment (type=%d).", (int) seg->datatype);
            break;
    }
}
#endif

/*--------------------------------------------------------------------------*/
/*! read the data header of this segment
   @param file (inout) file descriptor.
   @param filename (in) A character string giving the name of an ephemeris data
   file.
   @param seg (inout) segment
*/
/*--------------------------------------------------------------------------*/
static int calceph_spk_readsegment_header(FILE *file, const char *filename, struct SPKSegmentHeader *seg)
{
    double drecord[12];

    int res = 1;

    switch (seg->datatype)
    {
        case SPK_SEGTYPE1:
            res = calceph_spk_readword(file, filename, seg->rec_end, seg->rec_end, drecord);
            if (res == 1)
            {
                int nrecord, ndirectory;

                calceph_bff_convert_array_double(drecord, 1, seg->bff);

                nrecord = (int) drecord[0];
                ndirectory = nrecord;

                seg->seginfo.data1.count_record = nrecord;
                if (nrecord >= 100)
                    ndirectory = (nrecord / 100);
                seg->seginfo.data1.count_directory = ndirectory;
                res = calceph_spk_allocate_directory(seg->seginfo.data1.count_directory, &seg->seginfo.data1.directory);
                if (res == 1)
                    res = calceph_spk_readword(file, filename, seg->rec_end - 1 - ndirectory + 1, seg->rec_end - 1,
                                               seg->seginfo.data1.directory);
            }
            break;

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

        case SPK_SEGTYPE5:
            res = calceph_spk_readword(file, filename, seg->rec_end - 1, seg->rec_end, drecord);
            if (res == 1)
            {
                int nrecord, ndirectory;
                double GM;

                calceph_bff_convert_array_double(drecord, 2, seg->bff);

                GM = drecord[0];
                nrecord = (int) drecord[1];
                ndirectory = nrecord;

                seg->seginfo.data5.GM = GM;
                seg->seginfo.data5.count_record = nrecord;
                if (nrecord >= 100)
                    ndirectory = (nrecord / 100);
                seg->seginfo.data5.count_directory = ndirectory;
                res = calceph_spk_allocate_directory(seg->seginfo.data5.count_directory, &seg->seginfo.data5.directory);
                if (res == 1)
                    res = calceph_spk_readword(file, filename, seg->rec_end - 2 - ndirectory + 1, seg->rec_end - 2,
                                               seg->seginfo.data5.directory);
                calceph_bff_convert_array_double(seg->seginfo.data5.directory, ndirectory, seg->bff);
            }
            break;

        case SPK_SEGTYPE8:
        case SPK_SEGTYPE12:
            res = calceph_spk_readword(file, filename, seg->rec_end - 3, seg->rec_end, drecord);
            if (res == 1)
            {
                calceph_bff_convert_array_double(drecord, 4, seg->bff);
                seg->seginfo.data12.T_begin = drecord[0];
                seg->seginfo.data12.step_size = drecord[1];
                seg->seginfo.data12.window_sizem1 = (int) drecord[2];
                seg->seginfo.data12.degree = 2 * (seg->seginfo.data12.window_sizem1 + 1) - 1;
                seg->seginfo.data12.count_record = (int) drecord[3];
            }
            break;

        case SPK_SEGTYPE9:
        case SPK_SEGTYPE13:
            res = calceph_spk_readword(file, filename, seg->rec_end - 1, seg->rec_end, drecord);
            if (res == 1)
            {
                int nrecord, ndirectory;

                calceph_bff_convert_array_double(drecord, 2, seg->bff);

                nrecord = (int) drecord[1];
                ndirectory = nrecord;

                seg->seginfo.data13.window_sizem1 = (int) drecord[0];
                seg->seginfo.data13.degree = 2 * (seg->seginfo.data13.window_sizem1 + 1) - 1;
                seg->seginfo.data13.count_record = nrecord;
                if (nrecord > 100)
                {
                    ndirectory = (nrecord / 100);
                    if (ndirectory * 100 == nrecord)
                        ndirectory--;
                }
                seg->seginfo.data13.count_directory = ndirectory;
                res = calceph_spk_allocate_directory(seg->seginfo.data13.count_directory,
                                                     &seg->seginfo.data13.directory);
                if (res == 1)
                {
                    res = calceph_spk_readword(file, filename, seg->rec_end - 2 - ndirectory + 1, seg->rec_end - 2,
                                               seg->seginfo.data13.directory);
                    calceph_bff_convert_array_double(seg->seginfo.data13.directory, ndirectory, seg->bff);
                }
            }
            break;

        case SPK_SEGTYPE14:
            res = calceph_spk_readsegment_header_14(file, filename, seg);
            break;

        case SPK_SEGTYPE17:
            res = calceph_spk_readword(file, filename, seg->rec_end - 11, seg->rec_end, drecord);
            if (res == 1)
            {
                calceph_bff_convert_array_double(drecord, 12, seg->bff);
                seg->seginfo.data17.epoch = drecord[0];
                seg->seginfo.data17.a = drecord[1];
                seg->seginfo.data17.h = drecord[2];
                seg->seginfo.data17.k = drecord[3];
                seg->seginfo.data17.mean_longitude = drecord[4];
                seg->seginfo.data17.p = drecord[5];
                seg->seginfo.data17.q = drecord[6];
                seg->seginfo.data17.rate_longitude_periapse = drecord[7];
                seg->seginfo.data17.mean_longitude_rate = drecord[8];
                seg->seginfo.data17.longitude_ascending_node_rate = drecord[9];
                seg->seginfo.data17.ra_pole = drecord[10];
                seg->seginfo.data17.de_pole = drecord[11];
            }
            break;

        case SPK_SEGTYPE18:
            res = calceph_spk_readsegment_header_18(file, filename, seg);
            break;

        case SPK_SEGTYPE19:
            res = calceph_spk_readsegment_header_19(file, filename, seg);
            break;

        case SPK_SEGTYPE21:
            res = calceph_spk_readword(file, filename, seg->rec_end - 1, seg->rec_end, drecord);
            if (res == 1)
            {
                int nrecord, ndirectory, dlsize;

                calceph_bff_convert_array_double(drecord, 2, seg->bff);

                dlsize = (int) drecord[0];
                nrecord = (int) drecord[1];
                ndirectory = nrecord;

                seg->seginfo.data21.dlsize = dlsize;
                seg->seginfo.data21.count_record = nrecord;
                if (nrecord >= 100)
                    ndirectory = (nrecord / 100);
                seg->seginfo.data21.count_directory = ndirectory;
                res = calceph_spk_allocate_directory(seg->seginfo.data21.count_directory,
                                                     &seg->seginfo.data21.directory);
                if (res == 1)
                {
                    res = calceph_spk_readword(file, filename, seg->rec_end - 2 - ndirectory + 1, seg->rec_end - 2,
                                               seg->seginfo.data21.directory);
                    calceph_bff_convert_array_double(seg->seginfo.data21.directory, ndirectory, seg->bff);
                }
            }
            break;

        default:
            fatalerror("Unsupported segment (type=%d).", (int) seg->datatype);
            res = 0;
            break;
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*!
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.

     It computes position and velocity vectors for a selected
     planetary body from Chebyshev coefficients.

     return 1 on success
     return 0 on error
     return -1 if the segment and object is not found

  @param pspk (inout) SPK file
  @param seg (inout) segment
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired
  (Julian Date)
  @param Planet (out) position and velocity
         Planet has position in km
         Planet has velocity in km/sec
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                    struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType *Planet)
{
    int res = 0;

    /* we found the valid segment */
    switch (seg->datatype)
    {
        case SPK_SEGTYPE1:
            res = calceph_spk_interpol_PV_segment_1(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE103:
        case SPK_SEGTYPE102:
        case SPK_SEGTYPE3:
        case SPK_SEGTYPE2:
            res = calceph_spk_interpol_PV_segment_2(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE5:
            res = calceph_spk_interpol_PV_segment_5(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE8:
        case SPK_SEGTYPE12:
            res = calceph_spk_interpol_PV_segment_12(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE9:
        case SPK_SEGTYPE13:
            res = calceph_spk_interpol_PV_segment_13(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE14:
            res = calceph_spk_interpol_PV_segment_14(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE17:
            res = calceph_spk_interpol_PV_segment_17(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE18:
            res = calceph_spk_interpol_PV_segment_18(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE19:
            res = calceph_spk_interpol_PV_segment_19(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE120:
        case SPK_SEGTYPE20:
            res = calceph_spk_interpol_PV_segment_20(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        case SPK_SEGTYPE21:
            res = calceph_spk_interpol_PV_segment_21(pspk, seg, cache, TimeJD0, Timediff, Planet);
            break;

        default:
            fatalerror("Unsupported segment (type=%d).\n", (int) seg->datatype);
            break;
    }

#if DEBUG
    if (res)
    {
        printf("answer for %23.16E : \n", TimeJD0 + Timediff);
        calceph_stateType_debug(Planet);
    }
#endif
    return res;
}

/*--------------------------------------------------------------------------*/
/*! detect if the header has the same endian of the hardware.
if not, the header is converted to the endian of the hardware

  @param header (inout) header to process
*/
/*--------------------------------------------------------------------------*/
enum SPKBinaryFileFormat calceph_bff_detect(struct SPKHeader *header)
{
    union
    {
        unsigned char c[4];
        unsigned int i;
    } endian;

    enum SPKBinaryFileFormat reqconvert;

    reqconvert = BFF_NATIVE_IEEE;
    endian.i = 0x01020304;
    if (0x04 == endian.c[0])
    {
        /* hardware is little endian */
        if (strncmp("BIG-IEEE", header->bff, BFF_LEN) == 0)
        {
#if DEBUG
            printf("convert 'Binary File Format Identification' to little endian\n");
#endif
            reqconvert = BFF_BIG_IEEE;
        }
    }
    else if (0x01 == endian.c[0])
    {
        /* hardware is big endian */
        if (strncmp("LTL-IEEE", header->bff, BFF_LEN) == 0)
        {
#if DEBUG
            printf("convert 'Binary File Format Identification' to big endian\n");
#endif
            reqconvert = BFF_LTL_IEEE;
        }
    }

    if (reqconvert != BFF_NATIVE_IEEE)
    {
        calceph_spk_convertheader(header, reqconvert);
    }
    return reqconvert;
}

/*--------------------------------------------------------------------------*/
/*!
     convert a integer to the native number

  @param x (inout) value to convert
  @param reqconvert (in) type of data in the file
*/
/*--------------------------------------------------------------------------*/
static int calceph_bff_convert_int(int x, enum SPKBinaryFileFormat reqconvert)
{
    int r = 0;

    size_t j;

    size_t n = sizeof(int) / sizeof(char);

    switch (reqconvert)
    {
        case BFF_BIG_IEEE:
        case BFF_LTL_IEEE:
            for (j = 0; j < n; j++)
            {
                ((unsigned char *) &r)[j] = ((unsigned char *) &x)[n - 1 - j];
            }
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("CALCEPH does not handle this conversion format.\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return r;
}

/*--------------------------------------------------------------------------*/
/*!
     convert an array of double to the native number

  @param x (inout) array of n "double"
  @param n (inout) number of floating-points in the array
  @param reqconvert (in) type of data in the file
*/
/*--------------------------------------------------------------------------*/
void calceph_bff_convert_array_double(double *x, int n, enum SPKBinaryFileFormat reqconvert)
{
    union uswap
    {
        double x;
        unsigned char r[sizeof(double)];
    } ur;

    int k;

    size_t j;

    size_t w = sizeof(double) / sizeof(char);

    switch (reqconvert)
    {
        case BFF_BIG_IEEE:
        case BFF_LTL_IEEE:
            for (k = 0; k < n; k++)
            {
                for (j = 0; j < w; j++)
                {
                    ur.r[j] = ((unsigned char *) (x + k))[w - 1 - j];
                }
                x[k] = ur.x;
            }
            break;

        case BFF_NATIVE_IEEE:
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("CALCEPH does not handle this conversion format.\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*!
     reorder the array of integer previously order by
  calceph_bff_convert_array_double

  @param x (inout) array of n "int"
  @param n (inout) number of floating-points in the array
  @param reqconvert (in) type of data in the file
*/
/*--------------------------------------------------------------------------*/
void calceph_bff_reorder_array_int(int *x, int n, enum SPKBinaryFileFormat reqconvert)
{
    int k;

    size_t j;

    size_t w = sizeof(double) / sizeof(int);

    switch (reqconvert)
    {
        case BFF_BIG_IEEE:
        case BFF_LTL_IEEE:
            for (k = 0; k < n; k += w)
            {
                for (j = 0; j < w / 2; j++)
                {
                    int tmp = x[k + j];

                    x[k + j] = x[k + w - 1 - j];
                    x[k + w - 1 - j] = tmp;
                }
            }
            break;

        case BFF_NATIVE_IEEE:
            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("CALCEPH does not handle this conversion format.\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
}
