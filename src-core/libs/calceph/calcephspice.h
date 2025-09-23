/*-----------------------------------------------------------------*/
/*!
  \file calcephspice.h
  \brief private API for calceph library
         access and interpolate SPICE Ephemeris data.

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

#ifndef __CALCEPH_SPICE__
#define __CALCEPH_SPICE__

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_PTHREAD
#include <pthread.h>
#endif
#if HAVE_LOCALE_H
#include <locale.h>
#endif
#if HAVE_XLOCALE_H
#include <xlocale.h>
#endif

#include "calcephstatetype.h"

/*-----------------------------------------------------------------*/
/* private API */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/* constants */
/*-----------------------------------------------------------------*/
/*! number of bytes in a DAF record  */
#define DAF_RECORD_LEN 1024

/*! number of characters in ID WORD of SPK */
#define IDWORD_LEN 8

/*! number of characters in IFNAME of SPK */
#define IFNAME_LEN 60

/*! number of characters in BFF of SPK */
#define BFF_LEN 8

/*! number of padding characters after BFF of SPK */
#define PADBFF_LEN 603

/*! number of characters in FTP of SPK */
#define FTP_LEN 28

/*! number of padding characters after FTP of SPK */
#define PADFTP_LEN 297

/*! number of segment per record of SPK */
#define NSEGMENT_PERRECORD 25

/*! number of characters in SGEMENT ID of SPK */
#define SEGMENTID_LEN 40

/*! number of characters in FRAME of SPK */
#define FRAME_LEN 5

/*! number of word in the summary of SPK */
#define NS_SPK 5

/*! list of SPICE supported file */
enum SPICEfiletype
{
    DAF_SPK = 1,                /*!< binary SPK file */
    DAF_PCK,                    /*!< binary PCK file */
    TXT_PCK,                    /*!< text PCK file */
    TXT_FK                      /*!< text FK file */
};

/*! list of SPK data type
see "4.2 SPK Chebyshev Kernel Data Types" */
enum SPKdatatype
{
    SPK_SEGTYPE1 = 1,           /*!< Modified Difference Arrays.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE2 = 2,           /*!< Chebyshev polynomials for position. fixed length time intervals.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE3 = 3,           /*!< Chebyshev polynomials for position and velocity. fixed
                                   length time intervals. The time argument for these ephemerides is TDB */
    SPK_SEGTYPE5 = 5,           /*!< Discrete states (two body propagation).
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE8 = 8,           /*!< Lagrange Interpolation - Equal Time Steps.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE9 = 9,           /*!< Lagrange Interpolation - Unequal Time Steps.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE12 = 12,         /*!< Hermite Interpolation - Equal Time Steps.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE13 = 13,         /*!< Hermite Interpolation - Unequal Time Steps.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE14 = 14,         /*!< Chebyshev Polynomials - Unequal Time Steps.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE17 = 17,         /*!< Equinoctial Elements.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE18 = 18,         /*!< ESOC/DDID Hermite/Lagrange Interpolation.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE19 = 19,         /*!< ESOC/DDID Piecewise Interpolation.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE20 = 20,         /*!< Chebyshev polynomials for velocity. fixed length time
                                   intervals. The time argument for these ephemerides is TDB */
    SPK_SEGTYPE21 = 21,         /*!< Extended Modified Difference Arrays.
                                   The time argument for these ephemerides is TDB */
    SPK_SEGTYPE102 = 102,       /*!< Chebyshev polynomials for position. fixed length time intervals.
                                   The time argument for these ephemerides is TCB */
    SPK_SEGTYPE103 = 103,       /*!< Chebyshev polynomials for position and velocity. fixed length
                                   time intervals. The time argument for these ephemerides is TCB */
    SPK_SEGTYPE120 = 120        /*!< Chebyshev polynomials for velocity. fixed length time intervals.
                                   The time argument for these ephemerides is TCB */
};

/*! Binary File Format
see "3.2.2 The File Record" */
enum SPKBinaryFileFormat
{
    BFF_BIG_IEEE = 1,           /*!< big-endian order for numbers, different from the
                                   current hardware */
    BFF_LTL_IEEE = 2,           /*!< little-endian order for numbers, different from the
                                   current hardware */
    BFF_NATIVE_IEEE = 0         /*!< big or little-endian order for numbers but same as current
                                   hardware. not in standard, but allow faster processing.  */
};

/*! handle the decimal point for any locale : strtod is dependent to locale for the decimal point */
#if HAVE_STRTOD_L && ((HAVE_NEWLOCALE && defined(LC_NUMERIC_MASK)) || HAVE__CREATE_LOCALE)              \
                && HAVE_FREELOCALE && (HAVE_LOCALE_H || HAVE_XLOCALE_H)
/*! use strtod version without locale sensitivity */
#define USE_STRTOD_L 1
/*! structure of C locale */
struct calceph_locale
{
    locale_t dataclocale;       /*!< define the locale C to handle properly the decimal point  */
    int useclocale;             /*!< =1 if locale should be used for the conversion */
};
#else
/*! discard the usage of strtod_l */
#undef USE_STRTOD_L
/*! structure of C locale */
struct calceph_locale
{
    int useclocale;             /*!< always 0 */
};
#endif

/*-----------------------------------------------------------------*/
/* structures */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/*! binary SPK header */
/*-----------------------------------------------------------------*/
struct SPKHeader
{
    char idword[IDWORD_LEN];    /*!< Indicates file architecture and type */
    int nd, ni;                 /*!< Number of d.p. and integer descriptor components */
    char ifname[IFNAME_LEN];    /*!< Internal file name */
    int fwd, bwd;               /*!< Forward and backward linked list pointers */
    int free;                   /*!< First free DAF address */
    char bff[BFF_LEN];          /*!<  Binary file format ID */
    char padbff[PADBFF_LEN];    /*!< pad character after BFF */
    unsigned char ftp[FTP_LEN]; /*!<  FTP corruption test string */
    char padftp[PADFTP_LEN];    /*!< pad character after FTP */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 1  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_1
{
    int count_record;           /*!< number of records in segment */
    double *directory;          /*!< min(N,100) : fast directory if count_record>100
                                   or  explicit directory if N<=100 */
    int count_directory;        /*!< number of valid elements in  directory */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 2, 3, 102 or 103  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_2
{
    double T_begin;             /*!< initial epoch of first record */
    double T_len;               /*!< length of interval covered by each record */
    int count_dataperrecord;    /*!< number of data elements in each record */
    int count_record;           /*!< number of records in segment */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 5 */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_5
{
    int count_record;           /*!< number of records in segment */
    double *directory;          /*!< min(N,100) : fast directory if count_record>100
                                   or  explicit directory if N<=100 */
    double GM;                  /*!<  gravitational parameter of the central body (kilometers and seconds) */
    int count_directory;        /*!< number of valid elements in  directory */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 8 or 12  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_12
{
    int count_record;           /*!< number of records in segment */
    double T_begin;             /*!< epoch of the first record (expressed in TDB) */
    double step_size;           /*!< Step size */
    int window_sizem1;          /*!< window size - 1  */
    int degree;                 /*!< polynomial degree  */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 9 or 13  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_13
{
    int count_record;           /*!< number of records in segment */
    double *directory;          /*!< min(N,100) : fast directory if count_record>100
                                   or  explicit directory if N<=100 */
    int count_directory;        /*!< number of valid elements in  directory */
    int window_sizem1;          /*!< window size - 1  */
    int degree;                 /*!< polynomial degree  */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 14  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_14
{
    double fast_epochs[90];     /*!<  explicit content of reference epochs (count_reference_epochs elements) if
                                   count_reference_epochs<=90 or explicit content of epoch directory
                                   (count_directory_epoch elements) if count_directory_epoch<=90 or unused */
    int degree;                 /*!< polynomial degree  */
    int rec_packets;            /*!< record of the first packets */
    int count_packets;          /*!< number of the packets */
    int size_packet;            /*!< size of a packet (expressed in "double") */
    int offset_start_packet;    /*!< offset of the packet data from the start of a packet record */
    int rec_reference_epochs;   /*!< record of the first reference epochs */
    int count_reference_epochs; /*!< number of the reference epochs */
    int rec_directory_epoch;    /*!< record of the epoch directory */
    int count_directory_epoch;  /*!< number of the epochs in the epoch directory */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 17  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_17
{
    double epoch;               /*!< Epoch of Periapsis */
    double a;                   /*!< Semi-Major Axis  */
    double h;                   /*!< H term of equinoctial elements   */
    double k;                   /*!< K term of equinoctial elements   */
    double mean_longitude;      /*!< Mean longitude at epoch   */
    double p;                   /*!< P term of equinoctial elements   */
    double q;                   /*!< Q term of equinoctial elements   */
    double rate_longitude_periapse; /*!< rate of longitude of periapse   */
    double mean_longitude_rate; /*!< mean longitude rate   */
    double longitude_ascending_node_rate;   /*!< longitude of ascending node rate   */
    double ra_pole;             /*!< equatorial pole right ascension   */
    double de_pole;             /*!< equatorial pole declination   */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 18 */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_18
{
    int count_record;           /*!< number of records in segment */
    double *directory;          /*!< min(N,100) : fast directory if count_record>100
                                   or  explicit directory if N<=100 */
    int count_directory;        /*!< number of valid elements in  directory */
    int subtype;                /*!<  Subtype code  */
    int window_size;            /*!< window size   */
    int degree;                 /*!< polynomial degree  */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 19 */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_19
{
    double *boundary_times_directory;          /*!< min(N,100) : fast directory of times  if number_intervals>100
                                   or  explicit directory if N<=100 */
    int count_boundary_times_directory;        /*!< number of valid elements in  boundary_times_directory */
    double *boundary_times;          /*!< n+1 directory of times */
    double *mini_segments;      /*!< number_intervals+1 mini-segments pointer */
    int number_intervals;       /*!< number of intervals in segment */
    int boundary_choice_flag;   /*!<  boundary choice flag   */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 20 or 120  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_20
{
    double T_begin;             /*!< initial epoch of first record  =
                                   T_init_JD+T_init_FRACTION */
    double T_len;               /*!< length of interval covered by each record */
    int count_dataperrecord;    /*!< number of data elements in each record */
    int count_record;           /*!< number of records in segment */
    double T_init_JD;           /*!< integer part of initial epoch of first record  */
    double T_init_FRACTION;     /*!< fractional part of initial epoch of first record  */
    double dscale;              /*!< the distance scale in kilometers  */
    double tscale;              /*!< the time scale in seconds  */
};

/*-----------------------------------------------------------------*/
/*! Segment Header of type 21  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader_21
{
    int count_record;           /*!< number of records in segment */
    double *directory;          /*!< min(N,100) : fast directory if count_record>100
                                   or  explicit directory if N<=100 */
    int dlsize;                 /*!< Difference line size */
    int count_directory;        /*!< number of valid elements in  directory */
};

/*-----------------------------------------------------------------*/
/*! common Segment Header  for all SPK data type  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeader
{
    char id[SEGMENTID_LEN];     /*!< segment id */
    char descriptor[NS_SPK * 8];    /*!< segment descriptor */
    double T_begin;             /*!< begin of the segment in seconds */
    double T_end;               /*!< end of the segment in seconds */
    int body;                   /*!< body */
    int center;                 /*!< center */
    int refframe;               /*!< reference frame number */
    enum SPKdatatype datatype;  /*!< type of segment */
    int rec_begin;              /*!< first data record  of the segment */
    int rec_end;                /*!< last data record  of the segment */
    enum SPKBinaryFileFormat bff;   /*!< binary file format */
    union
    {
        struct SPKSegmentHeader_1 data1;    /*!< valid if datatype= SPK_SEGTYPE1 */
        struct SPKSegmentHeader_2 data2;    /*!< valid if datatype= SPK_SEGTYPE2 */
        struct SPKSegmentHeader_2 data3;    /*!< valid if datatype= SPK_SEGTYPE3 */
        struct SPKSegmentHeader_5 data5;    /*!< valid if datatype= SPK_SEGTYPE5 */
        struct SPKSegmentHeader_12 data12;  /*!< valid if datatype= SPK_SEGTYPE12 or SPK_SEGTYPE8 */
        struct SPKSegmentHeader_13 data13;  /*!< valid if datatype= SPK_SEGTYPE13 or SPK_SEGTYPE9 */
        struct SPKSegmentHeader_14 data14;  /*!< valid if datatype= SPK_SEGTYPE14 */
        struct SPKSegmentHeader_17 data17;  /*!< valid if datatype= SPK_SEGTYPE17 */
        struct SPKSegmentHeader_18 data18;  /*!< valid if datatype= SPK_SEGTYPE18 */
        struct SPKSegmentHeader_19 data19;  /*!< valid if datatype= SPK_SEGTYPE19 */
        struct SPKSegmentHeader_20 data20;  /*!< valid if datatype= SPK_SEGTYPE20 */
        struct SPKSegmentHeader_21 data21;  /*!< valid if datatype= SPK_SEGTYPE21 */
        struct SPKSegmentHeader_2 data102;  /*!< valid if datatype= SPK_SEGTYPE102 */
        struct SPKSegmentHeader_2 data103;  /*!< valid if datatype= SPK_SEGTYPE103 */
        struct SPKSegmentHeader_20 data120; /*!< valid if datatype= SPK_SEGTYPE120 */
    } seginfo;                  /*!< specific data for the segment */
};

/*-----------------------------------------------------------------*/
/*! array of NSEGMENT_PERRECORD Segment Header for  SPK  */
/*-----------------------------------------------------------------*/
struct SPKSegmentHeaderArray
{
    int count;                  /*!< number of valid segment */
    struct SPKSegmentHeader array[NSEGMENT_PERRECORD];  /*!< segments */
};

/*-----------------------------------------------------------------*/
/*! double-linked list of SPKSegmentHeaderArray */
/*-----------------------------------------------------------------*/
struct SPKSegmentList
{
    struct SPKSegmentList *prev;    /*!< previous element */
    struct SPKSegmentList *next;    /*!< next element */
    int recordnumber;           /*!< number of its record in the file */
    struct SPKSegmentHeaderArray array_seg; /*!< array of segments */
};

/*-----------------------------------------------------------------*/
/*! state structure for a SPK ephemeris  */
/*-----------------------------------------------------------------*/
struct SPKfile
{
    FILE *file;                 /*!< I/O descriptor */
    struct SPKHeader header;    /*!< header of the SPK file */
    struct SPKSegmentList *list_seg;    /*!< list of array of segments */
    /* following data for prefetching */
    int prefetch;               /*!< = 1 => the file is prefetched */
    double *mmap_buffer;        /*!< preallocated buffer with mmap */
    size_t mmap_size;           /*!< size of mmap_buffer in bytes */
    int mmap_used;              /*!< =1 mmap was used instead of malloc */
    enum SPKBinaryFileFormat bff;   /*!< if bff!=BFF_NATIVE_IEEE, disable prefetch */
};

/*-----------------------------------------------------------------*/
/*! list of value of a constant in a PCK ephemeris  */
/*-----------------------------------------------------------------*/
struct TXTPCKvalue
{
    struct TXTPCKvalue *next;   /*!< next value */
    char *buffer;               /*!< content of the constant */
    off_t locfirst;             /*!< location of the first character in the buffer */
    off_t loclast;              /*!< location of the last character in the buffer */
};

/*-----------------------------------------------------------------*/
/*! list of constant in a PCK ephemeris  */
/*-----------------------------------------------------------------*/
struct TXTPCKconstant
{
    struct TXTPCKconstant *next;    /*!< next constant */
    char *name;                 /*!< name */
    struct TXTPCKvalue *value;  /*!< value */
    int assignment;             /*!< incremental assignment = 1, direct assignment = 0 */
};

/*-----------------------------------------------------------------*/
/*! state structure for a text PCK ephemeris  */
/*-----------------------------------------------------------------*/
struct TXTPCKfile
{
    struct calceph_locale clocale;  /*!< C locale for the conversion : not duplicated, should not be free */
    char *buffer;               /*!< content of the constant */
    struct TXTPCKconstant *listconstant;    /*!< list of constants */
};

/*-----------------------------------------------------------------*/
/*! list of frame in a FK ephemeris  */
/*-----------------------------------------------------------------*/
struct TXTFKframe
{
    struct TXTFKframe *next;    /*!< next frame */
    char *name;                 /*!< name */
    struct TXTPCKvalue *frame_name; /*!< frame name */
    int frame_id;               /*!< frame id */
    int class_id;               /*!< class id */
    int classtype;              /*!< class  */
    int center;                 /*!< center */
    /* TK frame */
    struct TXTPCKvalue *tkframe_relative;   /*!< tk relative */
    int tkframe_relative_id;    /*!< tk relative frame id */
    struct TXTPCKvalue *tkframe_spec;   /*!< tk spec */
    double tkframe_matrix[9];   /*!< tk matrix */
    double tkframe_angles[3];   /*!< tk euler angles  */
    int tkframe_axes[3];        /*!< tk euler axes  */
    struct TXTPCKvalue *tkframe_unit;   /*!< tk unit of the euler angles  */
    struct TXTPCKvalue *tkframe_quaternion; /*!< tk quaternion */
};

/*-----------------------------------------------------------------*/
/*! state structure for a text FK ephemeris  */
/*-----------------------------------------------------------------*/
struct TXTFKfile
{
    struct TXTPCKfile txtpckfile;   /*!< list of constants */
    struct TXTFKframe *listframe;   /*!< list of frame */
};

/*-----------------------------------------------------------------*/
/*! state structure for a spice ephemeris file  */
/*-----------------------------------------------------------------*/
struct SPICEkernel
{
    enum SPICEfiletype filetype;    /*!< type of the ephemeris  */
    union
    {
        struct SPKfile spk;     /*!< SPK file, valid only if filetype = DAF_SPK */
        struct TXTPCKfile txtpck;   /*!< text PCK file, valid only if filetype = TXT_PCK */
        struct TXTFKfile txtfk; /*!< text FK file, valid only if filetype = TXT_FK */
    } filedata;                 /*!< content of the ephemeris */
    struct SPICEkernel *next;   /*!< next spice ephemeris file */
};

/*-----------------------------------------------------------------*/
/*! cache for each object : very small cache
  only one record is kept per object. so usually we use only about 500 bytes per
  object).
*/
/*-----------------------------------------------------------------*/
struct SPICEcache
{
    const struct SPKSegmentHeader *segment; /*!< cached segment */
    int rec_begin;              /*!< first word cached in the segment */
    int rec_end;                /*!< last word cached in the segment */
    double *buffer;             /*!< array of cached words */
    size_t buffer_size;         /*!< maximal number of elements stored in buffer */
};

/*-----------------------------------------------------------------*/
/*! link to connect object center with object target
for a single slice of time */
/*-----------------------------------------------------------------*/
struct SPICElinktime
{
    double T_begin;             /*!< first time (JD) */
    double T_end;               /*!< last time (JD) */
    int count_body;             /*!< number of objects to be evaluated */
    int *array_body;            /*!< array of "count_body"  id */
    struct SPKSegmentHeader **array_segment;    /*!< array of "count_body" segments */
    struct SPKfile **array_spkfile; /*!< array of "count_body" files */
    struct SPICEcache **array_cache;    /*!< array of "count_body" caches */
    double *array_factors;      /*!< array of "count_body" factors to apply to each
                                   evaluation (.e.g, *1 or *-1)  */
};

/*-----------------------------------------------------------------*/
/*! table to link all objects for the spice ephemeris files */
/*-----------------------------------------------------------------*/
struct SPICEtablelinkbody
{
    struct SPICElinktime **matrix_link; /*!< matrix of array to links */
    int *matrix_countlisttime;  /*!< matrix of number of elements in each entries
                                   of matrix_link   */
    int *array_spiceid;         /*!< array of "count_body" spice ids (allocated memory for "reserved_memory" spice
                                   ids) */
    int count_body;             /*!< number of objects */
    int reserved_memory;        /*!< number of reserved objects for the memory : e.g., array_spiceid is an array
                                   of "reserved_memory" ids but only "count_body" are valids */
    int line_size;              /*!< number of elemnt per line (equals to reserved_memory) */
    int initemrat;              /*!< =1 => earth-moon system is initialized */
    struct SPICEcache *array_cache; /*!< array of "count_body" cache (one per ids) (allocated memory for
                                       "reserved_memory" caches)  */
    int *shortestpathmaxlink;   /*!< array of "count_body" integers: each integer is the current maximal link
                                   of the body for the shortest path algorithm  (allocated memory for
                                   "reserved_memory" integers) */
#if HAVE_PTHREAD
    pthread_mutex_t mutex;      /*!< Pthreads mutex to protect the creation of link between two bodies */
#elif HAVE_WIN32API
    HANDLE mutex;               /*!< Windows mutex to protect the creation of link between two bodies */
#endif
};

/*-----------------------------------------------------------------*/
/*! state structure for a list of spice ephemeris files */
/*-----------------------------------------------------------------*/
struct calcephbin_spice
{
    struct SPICEkernel *list;   /*!<  list of the ephemeris file */
    struct SPICEtablelinkbody tablelink;    /*!< link table between objects */
    /* predefined constant */
    double AU;                  /*!< Astronomical Unit */
    double EMRAT;               /*!< Earth-Moon mass ratio */
    struct calceph_locale clocale;  /*!< C locale for the conversion : should be freed on close statement  */
};

/*-----------------------------------------------------------------*/
/*! array of ephemeris state structure */
/*-----------------------------------------------------------------*/

/*-----------------------------------------------------------------*/
/*! function */
/*-----------------------------------------------------------------*/
/*! open for reading a SPK file */
int calceph_spk_open(FILE * file, const char *filename, struct SPICEkernel *res);

/*! close for reading a SPK file */
void calceph_spk_close(struct SPKfile *eph);

/*! prefetch for reading a SPK file */
int calceph_spk_prefetch(struct SPKfile *eph);

/*! evaluate position/velocity of a body from a spice file */
int calceph_spice_compute_unit(struct calcephbin_spice *eph, double JD0, double time, int target,
                               int center, int unit, int order, double PV[ /*<=12 */ ]);

/*! evaluate orientation of a body from a spice file */
int calceph_spice_orient_unit(struct calcephbin_spice *eph, double JD0, double time, int target,
                              int unit, int order, double PV[ /*<=12 */ ]);

/*! evaluate position/velocity of a body from a spice file at several times */
int calceph_spice_compute_array_unit(struct calcephbin_spice *eph, int n, double *JD0, double *time,
                                     int target, int center, int unit, double *PV);

/*! evaluate orientation of a body from a spice file at several times */
int calceph_spice_orient_array_unit(struct calcephbin_spice *eph, int n, double *JD0, double *time,
                                    int target, int unit, double *PV);

/*! evaluate position/velocity inside a segment from a SPK file */
int calceph_spk_interpol_PV_segment(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                    struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);

/*! evaluate position/velocity of a body from a SPK file */
int calceph_spk_interpol_PV(struct calcephbin_spice *eph, struct SPKfile *pephbin,
                            struct SPICEcache *cache, treal TimeJD0, treal Timediff, int Target,
                            int Center, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 1 from a SPK file
 */
int calceph_spk_interpol_PV_segment_1(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                      struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 2,3,102 and 103
 * from a SPK file */
int calceph_spk_interpol_PV_segment_2(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                      struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 5 from a SPK file
 */
int calceph_spk_interpol_PV_segment_5(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                      struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 12 from a SPK file
 */
int calceph_spk_interpol_PV_segment_12(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 13 from a SPK file
 */
int calceph_spk_interpol_PV_segment_13(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 14 from a SPK file
 */
int calceph_spk_interpol_PV_segment_14(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 17 from a SPK file
 */
int calceph_spk_interpol_PV_segment_17(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 18 from a SPK file
 */
int calceph_spk_interpol_PV_segment_18(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 19 from a SPK file
 */
int calceph_spk_interpol_PV_segment_19(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);
/*! evaluate position/velocity of a body in a segment of type 20 and 120 from a
 * SPK file */
int calceph_spk_interpol_PV_segment_20(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);

/*! evaluate position/velocity of a body in a segment of type 21 from a SPK file */
int calceph_spk_interpol_PV_segment_21(struct SPKfile *pspk, struct SPKSegmentHeader *seg,
                                       struct SPICEcache *cache, treal TimeJD0, treal Timediff, stateType * Planet);

/*! evaluate position/velocity of a body using Hermite interpolation */
void calceph_spk_interpol_Hermite(int S, const double *drecord, const double *depoch, double TimeJD0,
                                  double Timediff, stateType * Planet);
/*! evaluate position/velocity of a body using Lagrange interpolation */
void calceph_spk_interpol_Lagrange(int S, const double *drecord, const double *depoch, double TimeJD0,
                                   double Timediff, stateType * Planet);

/*! evaluate position/velocity of a body using Chebychev interpolation */
void calceph_spk_interpol_Chebychev(int N, int ncomp, const double *Coeff_Array, treal TimeJD0,
                                    treal Timediff, stateType * Planet);

/*! read the header of the segment of type 14 froma SPK file */
int calceph_spk_readsegment_header_14(FILE * file, const char *filename, struct SPKSegmentHeader *seg);

/*! open for reading a binary PCK file */
int calceph_binpck_open(FILE * file, const char *filename, struct SPICEkernel *res);

/*! close for reading a binary PCK file */
void calceph_binpck_close(struct SPKfile *eph);

/*! open for reading a text PCK file */
int calceph_txtpck_open(FILE * file, const char *filename, struct calceph_locale clocale, struct SPICEkernel *res);

/*! close for reading a text PCK file */
void calceph_txtpck_close(struct TXTPCKfile *eph);

/*! open for reading a text FK file */
int calceph_txtfk_open(FILE * file, const char *filename, struct calceph_locale clocale, struct SPICEkernel *res);

/*! close for reading a text FK file */
void calceph_txtfk_close(struct TXTFKfile *eph);

/*! open for reading a text MK file */
int calceph_txtmk_open(FILE * file, const char *filename, struct calcephbin_spice *res);

/*! close for reading any SPICE file */
void calceph_spice_close(struct calcephbin_spice *eph);

/*! prefecth for reading any SPICE file */
int calceph_spice_prefetch(struct calcephbin_spice *eph);

/*! return a non-zero value if eph could be accessed by multiple threads */
int calceph_spice_isthreadsafe(const struct calcephbin_spice *eph);

/*! evaluate position/velocity of a body from a SPICE file */
int calceph_spice_interpol_PV(struct calcephbin_spice *pephbin, treal TimeJD0, treal Timediff,
                              int Target, int Center, int Unit, stateType * Planet);

/*! add a file to the list of kernel */
struct SPICEkernel *calceph_spice_addfile(struct calcephbin_spice *eph);

/*! convert id from calceph to spice */
int calceph_spice_convertid(struct calcephbin_spice *eph, int target);

/*! solve the Kepler equation */
double calceph_solve_kepler(double ml, double h, double k);

/* convert the units */
int calceph_spice_unit_convert_orient(stateType * Planet, int InputUnit, int OutputUnit);

/* same as strtod with c locale support */
double calceph_strtod(const char *nptr, char **endptr, struct calceph_locale clocale);

/*-----------------------------------------------------------------*/
/*! internal functions to read spk/binary pck files */
/*-----------------------------------------------------------------*/
#if DEBUG
void calceph_spk_debugheader(const struct SPKHeader *header);

void calceph_txtpck_debug(const struct TXTPCKfile *header);
#endif
int calceph_spk_ftp(const unsigned char ftpbuf[FTP_LEN]);

int calceph_spk_readword(FILE * file, const char *filename, int rec_begin, int rec_end, double *record);

int calceph_spk_fastreadword(struct SPKfile *pspk, const struct SPKSegmentHeader *seg,
                             struct SPICEcache *cache, const char *filename, int rec_begin, int rec_end,
                             const double **record);
enum SPKBinaryFileFormat calceph_bff_detect(struct SPKHeader *header);
void calceph_bff_convert_array_double(double *x, int n, enum SPKBinaryFileFormat reqconvert);
void calceph_bff_reorder_array_int(int *x, int n, enum SPKBinaryFileFormat reqconvert);

int calceph_spk_readsegment_header_18(FILE * file, const char *filename, struct SPKSegmentHeader *seg);
int calceph_spk_readsegment_header_19(FILE * file, const char *filename, struct SPKSegmentHeader *seg);

int calceph_spk_allocate_directory(int count_directory, double **directory);

/*-----------------------------------------------------------------*/
/*! internal functions to read text kernel files */
/*-----------------------------------------------------------------*/
int calceph_txtpck_load(FILE * file, const char *filename, const char *header,
                        struct calceph_locale clocale, struct TXTPCKfile *res);

int calceph_txtpck_getconstant_int(struct TXTPCKfile *eph, const char *name, int *p_pival);
int calceph_txtpck_getconstant_intmatrix1x3(struct TXTPCKfile *eph, const char *name, int *p_pival);

int calceph_txtpck_getconstant_int2(const struct TXTPCKconstant *listconstant, int *p_pival);

int calceph_txtpck_getconstant_matrix3x3(struct TXTPCKfile *eph, const char *name, double *p_pdval);
int calceph_txtpck_getconstant_matrix1x3(struct TXTPCKfile *eph, const char *name, double *p_pdval);

int calceph_txtpck_getconstant_txtpckvalue(struct TXTPCKfile *eph, const char *name,
                                           struct TXTPCKvalue **p_ptxtpckvalue);
struct TXTPCKconstant *calceph_txtpck_getptrconstant(struct TXTPCKfile *eph, const char *name);

int calceph_txtpck_merge_incrementalassignment(struct SPICEkernel *list, struct SPICEkernel *newkernel);

int calceph_txtpck_getvalue_char(struct TXTPCKvalue *listvalue, char *p_pszval);

int calceph_txtpck_cmpvalue(const struct TXTPCKvalue *value1, const struct TXTPCKvalue *value2);

int calceph_txtpck_cmpszvalue(struct TXTPCKvalue *value1, const char *value2);

int calceph_txtpck_orient_unit(const struct calcephbin_spice *eph, double JD0, double time,
                               int spicetarget, int unit, int order, double PV[ /*<=12 */ ]);

/*-----------------------------------------------------------------*/
/*! internal functions for frame kernel files */
/*-----------------------------------------------------------------*/
const struct TXTFKframe *calceph_txtfk_findframe(const struct TXTFKfile *eph, const struct TXTPCKconstant *cst);

const struct TXTFKframe *calceph_txtfk_findframe2(const struct TXTFKfile *eph, const struct TXTPCKvalue *name);

const struct TXTFKframe *calceph_txtfk_findframe_id(const struct TXTFKfile *eph, int id);
void calceph_txtfk_creatematrix_axes1(double matrix[3][3], const double A);
int calceph_txtfk_creatematrix_eulerangles(double tkframe_matrix[9], const double tkframe_angles[3],
                                           int tkframe_axes[3]);
int calceph_txtfk_createeulerangles_matrix(double tkframe_angles[3], double tkframe_matrix[3][3]);
void calceph_matrix3x3_prod(double M[3][3], double A[3][3], double B[3][3]);

/*----------------------------------------------------------------------------*/
/* get constants */
/*----------------------------------------------------------------------------*/
double calceph_spice_getEMRAT(struct calcephbin_spice *p_pbinfile);

double calceph_spice_getAU(struct calcephbin_spice *p_pbinfile);

int calceph_spice_getconstant_vd(const struct calcephbin_spice *p_pbinfile, const char *name,
                                 double *arrayvalue, int nvalue);

int calceph_spice_getconstant_vs(struct calcephbin_spice *p_pbinfile, const char *name,
                                 t_calcephcharvalue * arrayvalue, int nvalue);

int calceph_spice_getconstantcount(struct calcephbin_spice *eph);

int calceph_spice_getconstantindex(struct calcephbin_spice *eph, int index,
                                   char name[CALCEPH_MAX_CONSTANTNAME], double *value);

int calceph_txtpck_getconstant_vd(const struct TXTPCKfile *eph, const char *name, double *arrayvalue, int nvalue);

int calceph_txtpck_getconstantcount(struct TXTPCKfile *eph);

int calceph_txtpck_getconstantindex(struct TXTPCKfile *eph, int *index,
                                    char name[CALCEPH_MAX_CONSTANTNAME], double *value);

const struct TXTPCKconstant *calceph_spicekernel_getptrconstant(struct SPICEkernel *eph, const char *name);

/*----------------------------------------------------------------------------*/
/* get additional information */
/*----------------------------------------------------------------------------*/
int calceph_spice_getfileversion(struct calcephbin_spice *eph, char szversion[CALCEPH_MAX_CONSTANTVALUE]);
int calceph_spice_gettimescale(const struct calcephbin_spice *eph);
int calceph_spice_gettimespan(const struct calcephbin_spice *eph, double *firsttime, double *lasttime, int *continuous);
int calceph_spice_getpositionrecordcount(const struct calcephbin_spice *eph);
int calceph_spice_getpositionrecordindex(const struct calcephbin_spice *eph, int index, int *target,
                                         int *center, double *firsttime, double *lasttime, int *frame, int *segid);
int calceph_spice_getorientrecordcount(const struct calcephbin_spice *eph);
int calceph_spice_getorientrecordindex(const struct calcephbin_spice *eph, int index, int *target,
                                       double *firsttime, double *lasttime, int *frame, int *segid);

/*-----------------------------------------------------------------*/
/*! internal functions to link objects */
/*-----------------------------------------------------------------*/
void calceph_spice_tablelinkbody_init(struct SPICEtablelinkbody *table);

void calceph_spice_tablelinkbody_close(struct SPICEtablelinkbody *table);

int calceph_spice_tablelinkbody_addfile(struct calcephbin_spice *eph, struct SPICEkernel *newkernel);

int calceph_spice_tablelinkbody_createinitiallink(struct calcephbin_spice *eph);

int calceph_spice_tablelinkbody_compute(struct calcephbin_spice *eph, double JD0, double time,
                                        int target, int center, stateType * statetarget);

int calceph_spice_cache_init(struct SPICEcache **cache, int count_body);

int calceph_spice_tablelinkbody_locatelinktime_complex(struct SPICEtablelinkbody *table, int target,
                                                       int center, struct SPICElinktime ***link, int **countlisttime);

int calceph_spice_findframe_matrix(const struct calcephbin_spice *eph, int id, double JD0, double time,
                                   double matrix[3][3]);

#endif /*__CALCEPH_SPICE__*/
