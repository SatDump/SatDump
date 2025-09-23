/*-----------------------------------------------------------------*/
/*! 
  \file calcephspkseg14.c 
  \brief perform I/O on SPICE KERNEL ephemeris data file 
         for the segments of type 14
         interpolate SPICE KERNEL Ephemeris data.
         for the segments of type 14
         compute position and velocity from SPICE KERNEL ephemeris file
         for the segments of type 14.

  \author  M. Gastineau 
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris. 

   Copyright, 2023, CNRS
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

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephspice.h"
#include "util.h"
#include "calcephinternal.h"

/*! internal function : function for the tests purposes */
void calceph_tests_set_size_fast_epochs(int s);

/*-----------------------------------------------------------------*/
/*! enumeration of the "Meta Data Item" for the generic segment */
/*-----------------------------------------------------------------*/
enum SPKGenericSegment_MetaDataItem
{
    SPKGenericSegment_CONBAS = 1,   /*!<   base address of the constants */
    SPKGenericSegment_NCON = 2, /*!<   number of constants */
    SPKGenericSegment_RDRBAS = 3,   /*!<   base address  of the reference directory */
    SPKGenericSegment_NRDR = 4, /*!<   number of items in the reference directory */
    SPKGenericSegment_RDRTYP = 5,   /*!<   type of the reference directory */
    SPKGenericSegment_REFBAS = 6,   /*!<   base address of the reference items */
    SPKGenericSegment_NREF = 7, /*!<   number of reference items */
    SPKGenericSegment_PDRBAS = 8,   /*!<   base address  of the packet directory */
    SPKGenericSegment_NPDR = 9, /*!<   number of items in the packet directory */
    SPKGenericSegment_PDRTYP = 10,  /*!<   type of the packet directory */
    SPKGenericSegment_PKTBAS = 11,  /*!<   base address of the packets */
    SPKGenericSegment_NPKT = 12,    /*!<   number of packets */
    SPKGenericSegment_RSVBAS = 13,  /*!<   base address of the Reserved Area */
    SPKGenericSegment_NRSV = 14,    /*!<   number of items in the reserved area */
    SPKGenericSegment_PKTSZ = 15,   /*!<   size of the packets  */
    SPKGenericSegment_PKTOFF = 16,  /*!<   offset of the packet data from the start of a packet record. */
    SPKGenericSegment_NMETA = 17    /*!<   number of meta data items  */
};

/*-----------------------------------------------------------------*/
/*! size of the array "fast_epochs" : only set by the tests  */
/*-----------------------------------------------------------------*/
static int size_fast_epochs = 90;

/*--------------------------------------------------------------------------*/
/*! internal function (should never be called by the users): 
 set the limit value for the tests
 @param s (in) new size of size_fast_epochs
*/
/*--------------------------------------------------------------------------*/
void calceph_tests_set_size_fast_epochs(int s)
{
    size_fast_epochs = s;
}

/*--------------------------------------------------------------------------*/
/*! read the data header of this segment of type 14
   @param file (inout) file descriptor.
   @param filename (in) A character string giving the name of an ephemeris data
   file.
   @param seg (inout) segment
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_readsegment_header_14(FILE * file, const char *filename, struct SPKSegmentHeader *seg)
{

    const int max_metasize = SPKGenericSegment_NMETA;
    const int min_metasize = 15;
    int j;
    double drecord[12];
    int res = 1;
    int meta_size = 0;
    int offset_degree = -1;
    int rec_fast_epochs = 0;
    int count_fast_epochs = 0;

    /* get the meta size */
    res = calceph_spk_readword(file, filename, seg->rec_end, seg->rec_end, drecord);
    if (res == 1)
    {
        calceph_bff_convert_array_double(drecord, 1, seg->bff);
        meta_size = (int) drecord[0];
    }
    /* read the meta content */
    if (min_metasize <= meta_size && meta_size <= max_metasize)
    {
        int begin_offset;
        double drecord_metadata[SPKGenericSegment_NMETA];
        int meta_data[SPKGenericSegment_NMETA];

        res = calceph_spk_readword(file, filename, seg->rec_end - meta_size + 1, seg->rec_end, drecord_metadata);
        calceph_bff_convert_array_double(drecord_metadata, meta_size, seg->bff);
        for (j = 0; j < SPKGenericSegment_NMETA; j++)
            meta_data[j] = 0;
        for (j = 0; j < meta_size; j++)
        {
            meta_data[j] = (int) drecord_metadata[j];
        }
        begin_offset = seg->rec_begin;
        offset_degree = meta_data[SPKGenericSegment_CONBAS - 1] + begin_offset;
        seg->seginfo.data14.rec_directory_epoch = meta_data[SPKGenericSegment_RDRBAS - 1] + begin_offset;
        seg->seginfo.data14.count_directory_epoch = meta_data[SPKGenericSegment_NRDR - 1];
        seg->seginfo.data14.rec_reference_epochs = meta_data[SPKGenericSegment_REFBAS - 1] + begin_offset;
        seg->seginfo.data14.count_reference_epochs = meta_data[SPKGenericSegment_NREF - 1];
        seg->seginfo.data14.rec_packets = meta_data[SPKGenericSegment_PKTBAS - 1] + begin_offset;
        seg->seginfo.data14.count_packets = meta_data[SPKGenericSegment_NPKT - 1];
        seg->seginfo.data14.size_packet = meta_data[SPKGenericSegment_PKTSZ - 1];
        seg->seginfo.data14.offset_start_packet = meta_data[SPKGenericSegment_PKTOFF - 1];
    }
    else if (res == 1)
    {
        fatalerror("Unsupported old format for the SPK segment type 14 (meta data item size= %d)", meta_size);
    }

    /* early exit on errors */
    if (res == 0)
        return res;

    /* read the degree of the polynomials */
    res = calceph_spk_readword(file, filename, offset_degree, offset_degree, drecord);
    if (res == 1)
    {

        calceph_bff_convert_array_double(drecord, 1, seg->bff);
        seg->seginfo.data14.degree = (int) drecord[0];

        /* read the "reference epochs" if the "reference epochs" is small */
        if (0 < seg->seginfo.data14.count_reference_epochs &&
            seg->seginfo.data14.count_reference_epochs <= size_fast_epochs)
        {
            rec_fast_epochs = seg->seginfo.data14.rec_reference_epochs;
            count_fast_epochs = seg->seginfo.data14.count_reference_epochs;

        }
        /* read the "epochs directory" if the "epochs directory" is small */
        else if (0 < seg->seginfo.data14.count_directory_epoch &&
                 seg->seginfo.data14.count_directory_epoch <= size_fast_epochs)
        {
            rec_fast_epochs = seg->seginfo.data14.rec_directory_epoch;
            count_fast_epochs = seg->seginfo.data14.count_directory_epoch;
        }

        if (count_fast_epochs > 0)
        {
            res =
                calceph_spk_readword(file, filename, rec_fast_epochs, rec_fast_epochs + count_fast_epochs - 1,
                                     seg->seginfo.data14.fast_epochs);
            if (res == 1)
            {
                calceph_bff_convert_array_double(seg->seginfo.data14.fast_epochs, count_fast_epochs, seg->bff);
            }
        }
    }
    return res;
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function find the reference epoch from the array of reference epochs 
     which must be used to evaluate the given time
 
                                                                          
  @param Timesec (in) requested time 
  @param reference_epochs (in) array of reference epochs
  @param count_reference_epochs (in) number of elements in the array reference_epochs

*/
/*--------------------------------------------------------------------------*/
static int calceph_spk_find_reference_epochs(double Timesec, const double *reference_epochs, int count_reference_epochs)
{
    int j;

    if (Timesec >= reference_epochs[count_reference_epochs - 1])
        return count_reference_epochs - 1;
    for (j = 1; j <= count_reference_epochs - 1; j++)
    {
        if (Timesec < reference_epochs[j])
            return j - 1;
    }
    return count_reference_epochs - 1;
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function find the reference epoch from the array of directory of epochs
     which must be used to evaluate the given time

     return -1 on errors
 
  @param pspk (inout) SPK file  
  @param seg (in) segment where is located Timesec 
  @param cache (inout) cache for the object
  @param Timesec (in) requested time 
  @param directory_epoch (in) array of directory of epoch
  @param count_directory_epoch (in) number of elements in the array directory_epoch
  @param offset_directory_epoch (in) number of directory of epoch before this array
  
*/
/*--------------------------------------------------------------------------*/
static int calceph_spk_find_directory_epoch(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                            struct SPICEcache *cache, double Timesec, const double *directory_epoch,
                                            int count_directory_epoch, int offset_directory_epoch)
{
    int j;
    const double *reference_epochs;
    int index_directory = count_directory_epoch;
    int absolute_index_epoch;
    int wordT;
    int count_reference_epochs = 100;

    for (j = 0; j <= count_directory_epoch - 1; j++)
    {
        if (Timesec < directory_epoch[j])
        {
            index_directory = j;
            break;
        }
    }
    absolute_index_epoch = (offset_directory_epoch + index_directory) * 100 - 1;
    if (absolute_index_epoch < 0)
        absolute_index_epoch = 0;
    wordT = seg->seginfo.data14.rec_reference_epochs + absolute_index_epoch;

    /* compute the number of reference epochs to read */
    if (absolute_index_epoch + count_reference_epochs > seg->seginfo.data14.count_reference_epochs)
        count_reference_epochs = seg->seginfo.data14.count_reference_epochs % 100;

    if (calceph_spk_fastreadword(pspk, seg, cache, "", wordT, wordT + count_reference_epochs - 1, &reference_epochs) ==
        0)
    {
        return -1;
    }
    return absolute_index_epoch + calceph_spk_find_reference_epochs(Timesec, reference_epochs, count_reference_epochs);
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function find the reference epoch from the directory of epochs
     which must be used to evaluate the given time

     return -1 on errors
 
  @param pspk (inout) SPK file  
  @param seg (in) segment where is located Timesec 
  @param cache (inout) cache for the object
  @param Timesec (in) requested time 
*/
/*--------------------------------------------------------------------------*/
static int calceph_spk_browse_directory_epoch(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                              struct SPICEcache *cache, double Timesec)
{
    const double *directory_epoch;
    int remainder_directory_to_read = seg->seginfo.data14.count_directory_epoch;
    int wordbegin = seg->seginfo.data14.rec_directory_epoch;
    int finish = 0;
    int count_directory_already_read = 0;

    if (seg->seginfo.data14.count_directory_epoch == 0)
    {
        const double *reference_epochs;
        int count_reference_epochs;

        wordbegin = seg->seginfo.data14.rec_reference_epochs;
        count_reference_epochs = seg->seginfo.data14.count_reference_epochs;

        if (calceph_spk_fastreadword
            (pspk, seg, cache, "", wordbegin, wordbegin + count_reference_epochs - 1, &reference_epochs) == 0)
        {
            return -1;
        }
        return calceph_spk_find_reference_epochs(Timesec, reference_epochs, count_reference_epochs);
    }
    else
    {
        do
        {
            int wordend;
            int count_directory = 500;  /* keep a small buffer */

            if (count_directory > remainder_directory_to_read)
            {
                count_directory = remainder_directory_to_read;
                finish = 1;
            }
            wordend = wordbegin + count_directory - 1;
            if (calceph_spk_fastreadword(pspk, seg, cache, "", wordbegin, wordend, &directory_epoch) == 0)
            {
                return -1;
            }
            if (Timesec <= directory_epoch[count_directory - 1] || finish == 1)
                return calceph_spk_find_directory_epoch(pspk, seg, cache, Timesec, directory_epoch, count_directory,
                                                        count_directory_already_read);
            /* keep the last directory as the first one in the next iteration */
            count_directory_already_read += count_directory - 1;
            remainder_directory_to_read -= count_directory - 1;
            wordbegin = wordend;
        }
        while (finish == 0);
        fatalerror
            ("INTERNAL CALCEPH ERROR : please report this bug to the developers (with the code : calceph_spk_browse_directory_epoch)");
        return -1;
    }
}

/*--------------------------------------------------------------------------*/
/*!                                                      
     This function read the data block corresponding to the specified Time
     if this block isn't already in memory.
     
     This function works only over the specified segment.
     The segment must be of type 14 (seg->seginfo.data14 must be valid).
     
     It computes position and velocity vectors for a selected  
     planetary body from Chebyshev coefficients.          

                                                                          
  @param pspk (inout) SPK file  
  @param seg (in) segment where is located TimeJD0+timediff 
  @param cache (inout) cache for the object
  @param TimeJD0  (in) Time for which position is desired (Julian Date)
  @param Timediff  (in) offset time from TimeJD0 for which position is desired (Julian Date)
  @param Planet (out) position and velocity                                                                        
         Planet has position in km                                                
         Planet has velocity in km/sec
*/
/*--------------------------------------------------------------------------*/
int calceph_spk_interpol_PV_segment_14(struct SPKfile *pspk, struct SPKSegmentHeader *seg, struct SPICEcache *cache,
                                       treal TimeJD0, treal Timediff, stateType * Planet)
{
#if DEBUG
    int p;
#endif

    const double Timesec = ((TimeJD0 - 2.451545E+06) + Timediff) * 86400E0;
    const int ncomp = 6;
    const int nword = seg->seginfo.data14.size_packet;
    int wordbegin;
    int wordend;
    const double *drecord;

    int index_packet = -1;      /* index of the packet, start from 0 */

    /* check in the fast_epochs */
    /*  if the "reference epochs" is small */
    if (0 < seg->seginfo.data14.count_reference_epochs &&
        seg->seginfo.data14.count_reference_epochs <= size_fast_epochs)
    {
        index_packet = calceph_spk_find_reference_epochs(Timesec, seg->seginfo.data14.fast_epochs,
                                                         seg->seginfo.data14.count_reference_epochs);
    }
    /* if the "epochs directory" is small */
    else if (0 < seg->seginfo.data14.count_directory_epoch &&
             seg->seginfo.data14.count_directory_epoch <= size_fast_epochs)
    {
        index_packet = calceph_spk_find_directory_epoch(pspk, seg, cache, Timesec, seg->seginfo.data14.fast_epochs,
                                                        seg->seginfo.data14.count_directory_epoch, 0);
    }
    else
    {
        index_packet = calceph_spk_browse_directory_epoch(pspk, seg, cache, Timesec);
    }
    /* early exit on errors */
    if (index_packet < 0)
    {
        return 0;
    }
    /* read the packet which contents the requested time */
    wordbegin =
        seg->seginfo.data14.rec_packets + seg->seginfo.data14.offset_start_packet + index_packet * (nword +
                                                                                                    seg->seginfo.
                                                                                                    data14.offset_start_packet);
    wordend = wordbegin + nword - 1;
    if (calceph_spk_fastreadword(pspk, seg, cache, "", wordbegin, wordend, &drecord) == 0)
    {
        return 0;
    }

    calceph_spk_interpol_Chebychev(seg->seginfo.data14.degree, ncomp, drecord, (TimeJD0 - 2.451545E+06) * 86400E0,
                                   Timediff * 86400E0, Planet);

#if DEBUG
    /* print the record */
    printf("Timesec=%23.16E\n", Timesec);
    printf("read : %d %d : %d\n", wordbegin, wordend, nword);
    printf("Epoch=%23.16E range=%23.16E\n", drecord[0], drecord[1]);
    printf("Time packets=[%23.16E,%23.16E[\n", (drecord[0] - drecord[1]) / 86400E0,
           (drecord[0] + drecord[1]) / 86400E0);
    for (p = seg->seginfo.data14.offset_start_packet; p < nword; p++)
        printf("internal %d %23.16E\n", p, drecord[p]);

    calceph_stateType_debug(Planet);
    if (Timesec < drecord[0] - drecord[1] || drecord[0] + drecord[1] < Timesec)
        fatalerror("not interval!");
#endif

    return 1;
}
