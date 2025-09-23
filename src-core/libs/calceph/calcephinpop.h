/*-----------------------------------------------------------------*/
/*!
  \file calcephinpop.h
  \brief private API for calceph library
         access and interpolate INPOP and original JPL Ephemeris data.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2008-2024, CNRS
   email of the author : Mickael.Gastineau@obspm.fr

 History :
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

/*-----------------------------------------------------------------*/
/* private API */
/*-----------------------------------------------------------------*/
#ifndef __cplusplus
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#endif /*__cplusplus */

#include "calcephstatetype.h"

/*-----------------------------------------------------------------*/
/* constants */
/*-----------------------------------------------------------------*/
/* ephemeris numbers */
#define EPHEMERIS_DE200 200
#define EPHEMERIS_INPOP 100

/*-----------------------------------------------------------------*/
/* structures */
/*-----------------------------------------------------------------*/

/*-------------------------------------------------------------------------*/
/*! content of binary header records
 size of the structure and offset in bytes :
sizeof(struct recOneData)=2856
label=0
constName=252
timeData=2652
numConst=2676
AU=2680
EMRAT=2688
coeffPtr=2696
DENUM=2840
libratPtr=2844
*/
/*-------------------------------------------------------------------------*/

/* the next data structure must be packed : no alignment */
/* push current alignment to stack */
#if !(defined(__GNUC__) && defined(sun))
#if defined(__IBMC__) || defined(__SUNPRO_C)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif
#endif

/*-------------------------------------------------------------------------*/
/*! content of the beginning of thefirst record of the binary file.
This content is common to JPL and INPOP files
*/
/*-------------------------------------------------------------------------*/
typedef struct
{
    char label[3][84];       /*!< unused field */
    char constName[400][6]; /*!< constant name */
    double timeData[3];/*!<  begin/end/step  */
    int numConst;  /*!< number of elements of constName */
    double AU;     /*!< astronomical unit */
    double EMRAT;  /*!< earth-moon ratio */
    int coeffPtr[12][3]; /*!< pointer to the coefficients of the planets, ... */
    int DENUM;     /*!< ephemeris number */
    int libratPtr[3]; /*!< pointer to the coefficients of the libration */
}
#if defined(__GNUC__) || defined(__PGIC__)
__attribute__((packed))
#endif
    t_HeaderBlockBase;

/*-------------------------------------------------------------------------*/
/*! additional content of the first record of the binary file for the INPOP files */
/*-------------------------------------------------------------------------*/
typedef struct
{
    int recordsize;             /*!< size of the record in bytes (only in INPOP) */
    int TTmTDBPtr[3];           /*!< coefficients size/offset for TT-TDB (only in INPOP) */
}
#if defined(__GNUC__) || defined(__PGIC__)
__attribute__((packed))
#endif
    t_HeaderBlockExtensionInpop;

/*-------------------------------------------------------------------------*/
/*! additional content of the first record of the binary file for the JPL files
without additional constants (#constants>400) */
/*-------------------------------------------------------------------------*/
typedef struct
{
    /* additional constants (#constants>400)  should be here */
    int lunarAngularvelocityPtr[3];
    int TTmTDBPtr[3];
}
#if defined(__GNUC__) || defined(__PGIC__)
__attribute__((packed))
#endif
    t_HeaderBlockExtensionJPL;

/* restore original alignment from stack */
#if !(defined(__GNUC__) && defined(sun))
#if defined(__SUNPRO_C)
#pragma pack()
#else
#pragma pack(pop)
#endif
#endif

/*-------------------------------------------------------------------------*/
/*! content of the first record of the binary file for the JPL or INPOP files
 @details The order is not important in this data structure.
 */
/*-------------------------------------------------------------------------*/
typedef struct
{
    char label[3][84];      /*!< unused field */
    char constName[3000][6]; /*!< constant name */
    double timeData[3];/*!<  begin/end/step  */
    int numConst;  /*!< number of elements of constName */
    double AU;     /*!< astronomical unit */
    double EMRAT;  /*!< earth-moon ratio */
    int coeffPtr[12][3]; /*!< pointer to the coefficients of the planets, ... */
    int DENUM;     /*!< ephemeris number */
    int libratPtr[3]; /*!< pointer to the coefficients of the libration */
    int lunarAngularvelocityPtr[3]; /*!< pointer to the coefficients of the angular velocity of the Moon */
    int TTmTDBPtr[3];  /*!< pointer to the coefficients of the TT-TDB */
    int recordsize;    /*!< size of the records */
} t_HeaderBlock;

/*-------------------------------------------------------------------------*/
/*! content of the Asteroid  information  record of the binary file    */
/*-------------------------------------------------------------------------*/
typedef struct
{
    int locNextRecord;          /*!< location of the next information record.
                                   =0. no other information record. */
    int numAstRecord;           /*!< number of records used for asteroids in the file */
    int numAst;                 /*!< number of asteroids in the file */
    int typeAstRecord;          /*!< type of the tchebychev record
                                   !  =1. the intervals have a fixed length time and the asteroid time-slice is the
                                   same as the planet time-slice.  */
    int locIDAstRecord;         /*!< location of the first record used for the asteroid numbers . */
    int numIDAstRecord;         /*!< number of records used for the  asteroid numbers  */
    int locGMAstRecord;         /*!< location of the first record used for the value of the GM of the asteroids .
                                 */
    int numGMAstRecord;         /*!< number of records used for the value of the GM of the asteroids  */
    int locCoeffPtrAstRecord;   /*!< location of the first record used for the location, number of
                                   coefficients and  number of granules for the asteroids */
    int numCoeffPtrAstRecord;   /*!< number of records used  for location, number of coefficients and  number
                                   of granules for the asteroids */
    int locCoeffAstRecord;      /*!< location of the first record used for the asteroids' coefficients.  */
    int numCoeffAstRecord;      /*!< number of records used for the asteroids' coefficients over one time-slice.
                                 */
} t_InfoAstRecord;

/*-------------------------------------------------------------------------*/
/*! define the location, and number of coeffciients and the granularity   */
/*-------------------------------------------------------------------------*/
typedef int t_CoeffPtr[3];

/*-------------------------------------------------------------------------*/
/*! memory representation of one record  */
/*-------------------------------------------------------------------------*/
typedef struct
{
    FILE *file;                 /*!< binary ephemeris file */
    double *Coeff_Array;        /*!< array of coefficients */
    double T_beg;               /*!< time of the first data in  Coeff_Array */
    double T_end;               /*!< time of the last data in Coeff_Array */
    double T_span;              /*!< time/length of a granule Coeff_Array */
    off_t offfile;              /*!< current offset in the ephemeris file */
    int ARRAY_SIZE;             /*!< number of coefficients in a record */
    int ncomp;                  /*!< number of component in the  file */
    int ncompTTmTDB;            /*!< number of component in the  file for the TTmTDB */
    int swapbyteorder;          /*!< = true : swap the bytes after each read operation */
    /* following data for prefetching */
    double *mmap_buffer;        /*!< preallocated buffer with mmap */
    size_t mmap_size;           /*!< size of mmap_buffer in bytes */
    int mmap_used;              /*!< = 1 if mmap was used instead of malloc */
    int prefetch;               /*!< = 1 => the file is prefetched */
} t_memarcoeff;

/*-------------------------------------------------------------------------*/
/*! manage asteroids in the binary file  */
/*-------------------------------------------------------------------------*/
typedef struct
{
    t_InfoAstRecord inforec;    /*!< content of the asteroid information record */
    t_memarcoeff coefftime_array;   /*!< array of coefficients for asteroids with useful information */
    int *id_array;              /*!< array of id for asteroids */
    double *GM_array;           /*!< array of GM for asteroids */
    t_CoeffPtr *coeffptr_array; /*!< array of coefficient pointer for asteroids */
} t_ast_calcephbin;

/*-------------------------------------------------------------------------*/
/*! content of the Planetary Angular Momentum information record
of the binary file */
/*-------------------------------------------------------------------------*/
typedef struct
{
    int locNextRecord;          /*!< location of the next information record.
                                   =0. no other information record. */
    int numAngMomPlaRecord;     /*!< number of records used for angular momentum of the planets in the file */
    int nCompAngMomPla;         /*!<  number of components used for each angular momentum of the planets */
    int locCoeffAngMomPlaRecord;    /*!<  location of the first record used for the coefficients of the
                                       angular momemtum of the planets. */
    int coeffPtr[12][3];        /*!< location, number of coefficients and number of granules for the angular
                                   momemtum of the planets inside a record */
    int sizeAngMomPlaRecord;    /*!< record size of the angular momentum of the planets (unit: 64-bit
                                   floating-point numbers).  */
} t_InfoPamRecord;

/*-------------------------------------------------------------------------*/
/*! manage the Planetary Angular Momentum in the binary file  */
/*-------------------------------------------------------------------------*/
typedef struct
{
    t_InfoPamRecord inforec;    /*!< content of the Planetary Angular Momentum information record */
    t_memarcoeff coefftime_array;   /*!< array of coefficients for Planetary Angular Momentum with useful
                                       information */
} t_pam_calcephbin;

/*-------------------------------------------------------------------------*/
/*! time scale enumeration */
/*-------------------------------------------------------------------------*/
typedef enum etimescale
{
    etimescale_TDB = 0,         /*!<  ephemeris expressed in TDB time scale */
    etimescale_TCB = 1          /*!<  ephemeris expressed in TCB time scale */
} t_etimescale;

/*-------------------------------------------------------------------------*/
/*  Structure globale                                                       */
/*-------------------------------------------------------------------------*/
/*! ephemeris descriptor for the binary file JPL/INPOP */
struct calcephbin_inpop
{
    t_HeaderBlock H1;           /*!< contenu du 1er record du fichier */
    t_memarcoeff coefftime_array;   /*!< array of coefficients for planets with useful information */
    double constVal[3000];      /*!< valeur des constantes */
    bool isinAU;                /*!< = true : coefficients are in UA (else in km) */
    bool isinDay;               /*!< = true : coefficients are in /day (else in sec) except tt-tdb or tcg-tcb */
    bool haveTTmTDB;            /*!< = true : TT-TDB or TCG-TCB is in the file  */
    bool haveNutation;          /*!< = true : 1980 IAU nutation angles is in the file */
    bool haveLunarAngularVelocity;  /*!< = true : Lunar angular velocity is in the file */
    t_etimescale timescale;     /*!< time scale of the file */
    t_ast_calcephbin asteroids; /*!< asteroids information */
    t_pam_calcephbin pam;       /*!< planetary angular momentum information */
};

/*----------------------------------------------------------------------------*/
/* functions */
/*----------------------------------------------------------------------------*/
void calceph_interpol_PV_lowlevel(stateType * X, const treal * A, treal Tc, treal scale, int N, int ncomp);
int calceph_inpop_interpol_PV(struct calcephbin_inpop *p_pbinfile, treal TimeJD0, treal timediff,
                              int Target, int Center, int Unit, stateType * Planet);
int calceph_inpop_readcoeff(t_memarcoeff * pcoeftime_array, double Time);
int calceph_inpop_seekreadcoeff(t_memarcoeff * pephbin, double Time);
int calceph_inpop_open(FILE * file, const char *fileName, struct calcephbin_inpop *res);
struct calcephbin_inpop calceph_inpop_fdopen(struct calcephbin_inpop *eph);
void calceph_inpop_close(struct calcephbin_inpop *eph);
int calceph_inpop_prefetch(struct calcephbin_inpop *eph);
void calceph_interpol_PV_an(const t_memarcoeff * p_pbinfile, treal TimeJD0, treal Timediff,
                            stateType * Planet, int C, int G, int N, int ncomp);

void calceph_empty_asteroid(t_ast_calcephbin * p_pasteph);
int calceph_init_asteroid(t_ast_calcephbin * p_pasteph, FILE * file, int swapbyte, double timeData[3],
                          int recordsize, int ncomp, int *locnextrecord);
void calceph_free_asteroid(t_ast_calcephbin * p_pasteph);
int calceph_interpol_PV_asteroid(t_ast_calcephbin * p_pbinfile, treal TimeJD0, treal Timediff, int Target,
                                 int *ephunit, int isinAU, stateType * Planet);

void calceph_empty_pam(t_pam_calcephbin * p_ppameph);
int calceph_init_pam(t_pam_calcephbin * p_pasteph, FILE * file, int swapbyte, double timeData[3],
                     int recordsize, int *locnextrecord);
void calceph_free_pam(t_pam_calcephbin * p_pasteph);

/*! evaluate position/velocity of a body from a inpop file at one time */
int calceph_inpop_compute_unit(t_calcephbin * pephbin, double JD0, double time, int target, int center,
                               int unit, int order, double PV[ /*<=12 */ ]);
/*! evaluate orientation of a body from a inpop file at one time */
int calceph_inpop_orient_unit(t_calcephbin * pephbin, double JD0, double time, int target, int unit,
                              int order, double PV[ /*<=12 */ ]);
/*! evaluate the rotational angular momentum G/(mR^2) of a body from a inpop file at one time */
int calceph_inpop_rotangmom_unit(t_calcephbin * pephbin, double JD0, double time, int target, int unit,
                                 int order, double PV[ /*<=12 */ ]);

/* check the values */
int calceph_inpop_compute_unit_check(int target, int center, int unit, int *target_oldid,
                                     int *center_oldid, int *newunit);
int calceph_interpol_PV_planet_check(const struct calcephbin_inpop *p_pbinfile, int Target, int *ephunit,
                                     int *pC, int *pG, int *pN, int *pncomp);

/*----------------------------------------------------------------------------*/
/* get constants */
/*----------------------------------------------------------------------------*/
double calceph_inpop_getEMRAT(struct calcephbin_inpop *p_pbinfile);
double calceph_inpop_getAU(struct calcephbin_inpop *p_pbinfile);
int calceph_inpop_getconstant(struct calcephbin_inpop *p_pbinfile, const char *name, double *p_pdval);
int calceph_inpop_getconstantindex(struct calcephbin_inpop *eph, int index,
                                   char name[CALCEPH_MAX_CONSTANTNAME], double *value);
int calceph_inpop_getconstantcount(struct calcephbin_inpop *eph);

/*----------------------------------------------------------------------------*/
/* get additional information */
/*----------------------------------------------------------------------------*/
int calceph_inpop_getfileversion(struct calcephbin_inpop *eph, char szversion[CALCEPH_MAX_CONSTANTVALUE]);
int calceph_inpop_gettimescale(struct calcephbin_inpop *eph);
int calceph_inpop_gettimespan(struct calcephbin_inpop *eph, double *firsttime, double *lasttime, int *continuous);
int calceph_inpop_getpositionrecordcount(struct calcephbin_inpop *eph);
int calceph_inpop_getpositionrecordindex(struct calcephbin_inpop *eph, int index, int *target,
                                         int *center, double *firsttime, double *lasttime, int *frame, int *segid);
int calceph_inpop_getorientrecordcount(struct calcephbin_inpop *eph);
int calceph_inpop_getorientrecordindex(struct calcephbin_inpop *eph, int index, int *target,
                                       double *firsttime, double *lasttime, int *frame, int *segid);
