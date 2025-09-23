/*-----------------------------------------------------------------*/
/*!
  \file calceph.h
  \brief public API for calceph library
        access and interpolate INPOP and JPL Ephemeris data.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2008-2025, CNRS
   email of the author : Mickael.Gastineau@obspm.fr
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

#ifndef __CALCEPH_H__
#define __CALCEPH_H__

/*----------------------------------------------------------------------------------------------*/
/* definition of the CALCEPH library version */
/*----------------------------------------------------------------------------------------------*/
/*! version : major number of CALCEPH library */
#define CALCEPH_VERSION_MAJOR 4
/*! version : minor number of CALCEPH library */
#define CALCEPH_VERSION_MINOR 0
/*! version : patch number of CALCEPH library */
#define CALCEPH_VERSION_PATCH 5

/*----------------------------------------------------------------------------------------------*/
/* windows specific support */
/*----------------------------------------------------------------------------------------------*/
/* Support for WINDOWS Dll:
   Check if we are inside a CALCEPH build, and if so export the functions.
   Otherwise does the same thing as CALCEPH */
#if defined(__GNUC__)
#define __CALCEPH_DECLSPEC_EXPORT __declspec(__dllexport__)
#define __CALCEPH_DECLSPEC_IMPORT __declspec(__dllimport__)
#endif
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define __CALCEPH_DECLSPEC_EXPORT __declspec(dllexport)
#define __CALCEPH_DECLSPEC_IMPORT __declspec(dllimport)
#endif
#ifdef __WATCOMC__
#define __CALCEPH_DECLSPEC_EXPORT __export
#define __CALCEPH_DECLSPEC_IMPORT __import
#endif
#ifdef __IBMC__
#define __CALCEPH_DECLSPEC_EXPORT _Export
#define __CALCEPH_DECLSPEC_IMPORT _Import
#endif

#if defined(__CALCEPH_LIBCALCEPH_DLL)
/* compile as a dll */
#if defined(__CALCEPH_WITHIN_CALCEPH)
#define __CALCEPH_DECLSPEC __CALCEPH_DECLSPEC_EXPORT
#else
#define __CALCEPH_DECLSPEC __CALCEPH_DECLSPEC_IMPORT
#endif
#else
/* other cases */
#define __CALCEPH_DECLSPEC
#endif /*__CALCEPH_LIBCALCEPH_DLL*/

/*----------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C"
{
#endif /*defined (__cplusplus) */

/*----------------------------------------------------------------------------------------------*/
/* definition of some constants */
/*----------------------------------------------------------------------------------------------*/
/*!\internal  using the pre-processor , cat the string version of CALCEPH
 * library (private API : users must not call this preprocessor define) */
#define CALCEPH_VERSION_BUILDERSTRING_CAT1(major, minor, patch) major##.##minor##.##patch
/*!\internal cat the string version of CALCEPH library (private API : users must
 * not call this preprocessor define) */
#define CALCEPH_VERSION_BUILDERSTRING_CAT(major, minor, patch)                                          \
  CALCEPH_VERSION_BUILDERSTRING_CAT1(major, minor, patch)

/*!\internal using the pre-processor , build the string version of CALCEPH
 * library (private API : users must not call this preprocessor define) */
#define CALCEPH_VERSION_BUILDERSTRINGIZE(version) #version
/*!\internal build the string version of CALCEPH library (private API : users
 * must not call this preprocessor define) */
#define CALCEPH_VERSION_BUILDERSTRING(version) CALCEPH_VERSION_BUILDERSTRINGIZE(version)

/*!  string version of CALCEPH library */
#define CALCEPH_VERSION_STRING                                                                          \
  CALCEPH_VERSION_BUILDERSTRING(CALCEPH_VERSION_BUILDERSTRING_CAT(                                      \
                  CALCEPH_VERSION_MAJOR, CALCEPH_VERSION_MINOR, CALCEPH_VERSION_PATCH))

/*! define the maximum number of characters (including the trailing '\0')
 that the name of a constant could contain. */
#define CALCEPH_MAX_CONSTANTNAME 33

/*! define the maximum number of characters (including the trailing '\0')
 that the value of a constant could contain. */
#define CALCEPH_MAX_CONSTANTVALUE 1024

/*! define the offset value for asteroid for calceph_?compute */
#define CALCEPH_ASTEROID 2000000

/* unit for the output */
/*! outputs are in Astronomical Unit */
#define CALCEPH_UNIT_AU 1
/*! outputs are in kilometers */
#define CALCEPH_UNIT_KM 2
/*! outputs are in day */
#define CALCEPH_UNIT_DAY 4
/*! outputs are in seconds */
#define CALCEPH_UNIT_SEC 8
/*! outputs are in radians */
#define CALCEPH_UNIT_RAD 16

/*! use the NAIF body identification numbers for target and center integers */
#define CALCEPH_USE_NAIFID 32

/* kind of output */
/*! outputs are the euler angles */
#define CALCEPH_OUTPUT_EULERANGLES 64
/*! outputs are the nutation angles */
#define CALCEPH_OUTPUT_NUTATIONANGLES 128


/* list of the known segment type for spice kernels and inpop/jpl original file format  */
/* segment of the original DE/INPOP file format */
#define CALCEPH_SEGTYPE_ORIG_0 0 /*!< segment type for the original DE/INPOP file format */
/* segment of the spice kernels */
#define CALCEPH_SEGTYPE_SPK_1                                                                           \
  1 /*!< Modified Difference Arrays. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_2                                                                           \
  2 /*!< Chebyshev polynomials for position. fixed length time intervals.The time argument for these    \
       ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_3                                                                           \
  3 /*!< Chebyshev polynomials for position and velocity. fixed length time intervals. The time         \
       argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_5                                                                           \
  5 /*!< Discrete states (two body propagation).  The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_8                                                                           \
  8 /*!< Lagrange Interpolation - Equal Time Steps. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_9                                                                           \
  9 /*!< Lagrange Interpolation - Unequal Time Steps. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_12                                                                          \
  12 /*!< Hermite Interpolation - Equal Time Steps. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_13                                                                          \
  13 /*!< Hermite Interpolation - Unequal Time Steps. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_14                                                                          \
  14 /*!< Chebyshev Polynomials - Unequal Time Steps. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_17                                                                          \
  17 /*!< Equinoctial Elements. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_18                                                                          \
  18 /*!< ESOC/DDID Hermite/Lagrange Interpolation.  The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_19                                                                          \
  19 /*!< ESOC/DDID Piecewise Interpolation. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_20                                                                          \
  20 /*!< Chebyshev polynomials for velocity. fixed length time intervals. The time argument for these  \
        ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_21                                                                          \
  21 /*!< Extended Modified Difference Arrays. The time argument for these ephemerides is TDB */
#define CALCEPH_SEGTYPE_SPK_102                                                                         \
  102 /*!< Chebyshev polynomials for position. fixed length time intervals.  The time argument for      \
         these ephemerides is TCB */
#define CALCEPH_SEGTYPE_SPK_103                                                                         \
  103 /*!< Chebyshev polynomials for position and velocity. fixed length time intervals. The time       \
         argument for these ephemerides is TCB */
#define CALCEPH_SEGTYPE_SPK_120                                                                         \
  120 /*!< Chebyshev polynomials for velocity. fixed length time intervals.  The time argument for      \
         these ephemerides is TCB */


/*! NAIF identification numbers for the Sun and planetary barycenters (table 2
 * of reference 1) */
#define NAIFID_SOLAR_SYSTEM_BARYCENTER 0
#define NAIFID_MERCURY_BARYCENTER 1
#define NAIFID_VENUS_BARYCENTER 2
#define NAIFID_EARTH_MOON_BARYCENTER 3
#define NAIFID_MARS_BARYCENTER 4
#define NAIFID_JUPITER_BARYCENTER 5
#define NAIFID_SATURN_BARYCENTER 6
#define NAIFID_URANUS_BARYCENTER 7
#define NAIFID_NEPTUNE_BARYCENTER 8
#define NAIFID_PLUTO_BARYCENTER 9
#define NAIFID_SUN 10

/*! NAIF identification numbers for the Coordinate Time ephemerides */
/*! value to set as the center to get any Coordinate Time */
#define NAIFID_TIME_CENTER 1000000000
/*! value to set as the target to get the Coordinate Time TT-TDB */
#define NAIFID_TIME_TTMTDB 1000000001
/*! value to set as the target to get the Coordinate Time TCG-TCB */
#define NAIFID_TIME_TCGMTCB 1000000002

/*! NAIF identification numbers for the planet centers and satellites (table 3
 * of reference 1)  */
#define NAIFID_MERCURY 199
#define NAIFID_VENUS 299
#define NAIFID_EARTH 399
#define NAIFID_MOON 301

#define NAIFID_MARS 499
#define NAIFID_PHOBOS 401
#define NAIFID_DEIMOS 402

#define NAIFID_JUPITER 599
#define NAIFID_IO 501
#define NAIFID_EUROPA 502
#define NAIFID_GANYMEDE 503
#define NAIFID_CALLISTO 504
#define NAIFID_AMALTHEA 505
#define NAIFID_HIMALIA 506
#define NAIFID_ELARA 507
#define NAIFID_PASIPHAE 508
#define NAIFID_SINOPE 509
#define NAIFID_LYSITHEA 510
#define NAIFID_CARME 511
#define NAIFID_ANANKE 512
#define NAIFID_LEDA 513
#define NAIFID_THEBE 514
#define NAIFID_ADRASTEA 515
#define NAIFID_METIS 516
#define NAIFID_CALLIRRHOE 517
#define NAIFID_THEMISTO 518
#define NAIFID_MEGACLITE 519
#define NAIFID_TAYGETE 520
#define NAIFID_CHALDENE 521
#define NAIFID_HARPALYKE 522
#define NAIFID_KALYKE 523
#define NAIFID_IOCASTE 524
#define NAIFID_ERINOME 525
#define NAIFID_ISONOE 526
#define NAIFID_PRAXIDIKE 527
#define NAIFID_AUTONOE 528
#define NAIFID_THYONE 529
#define NAIFID_HERMIPPE 530
#define NAIFID_AITNE 531
#define NAIFID_EURYDOME 532
#define NAIFID_EUANTHE 533
#define NAIFID_EUPORIE 534
#define NAIFID_ORTHOSIE 535
#define NAIFID_SPONDE 536
#define NAIFID_KALE 537
#define NAIFID_PASITHEE 538
#define NAIFID_HEGEMONE 539
#define NAIFID_MNEME 540
#define NAIFID_AOEDE 541
#define NAIFID_THELXINOE 542
#define NAIFID_ARCHE 543
#define NAIFID_KALLICHORE 544
#define NAIFID_HELIKE 545
#define NAIFID_CARPO 546
#define NAIFID_EUKELADE 547
#define NAIFID_CYLLENE 548
#define NAIFID_KORE 549
#define NAIFID_HERSE 550
#define NAIFID_DIA 553

#define NAIFID_SATURN 699
#define NAIFID_MIMAS 601
#define NAIFID_ENCELADUS 602
#define NAIFID_TETHYS 603
#define NAIFID_DIONE 604
#define NAIFID_RHEA 605
#define NAIFID_TITAN 606
#define NAIFID_HYPERION 607
#define NAIFID_IAPETUS 608
#define NAIFID_PHOEBE 609
#define NAIFID_JANUS 610
#define NAIFID_EPIMETHEUS 611
#define NAIFID_HELENE 612
#define NAIFID_TELESTO 613
#define NAIFID_CALYPSO 614
#define NAIFID_ATLAS 615
#define NAIFID_PROMETHEUS 616
#define NAIFID_PANDORA 617
#define NAIFID_PAN 618
#define NAIFID_YMIR 619
#define NAIFID_PAALIAQ 620
#define NAIFID_TARVOS 621
#define NAIFID_IJIRAQ 622
#define NAIFID_SUTTUNGR 623
#define NAIFID_KIVIUQ 624
#define NAIFID_MUNDILFARI 625
#define NAIFID_ALBIORIX 626
#define NAIFID_SKATHI 627
#define NAIFID_ERRIAPUS 628
#define NAIFID_SIARNAQ 629
#define NAIFID_THRYMR 630
#define NAIFID_NARVI 631
#define NAIFID_METHONE 632
#define NAIFID_PALLENE 633
#define NAIFID_POLYDEUCES 634
#define NAIFID_DAPHNIS 635
#define NAIFID_AEGIR 636
#define NAIFID_BEBHIONN 637
#define NAIFID_BERGELMIR 638
#define NAIFID_BESTLA 639
#define NAIFID_FARBAUTI 640
#define NAIFID_FENRIR 641
#define NAIFID_FORNJOT 642
#define NAIFID_HATI 643
#define NAIFID_HYROKKIN 644
#define NAIFID_KARI 645
#define NAIFID_LOGE 646
#define NAIFID_SKOLL 647
#define NAIFID_SURTUR 648
#define NAIFID_ANTHE 649
#define NAIFID_JARNSAXA 650
#define NAIFID_GREIP 651
#define NAIFID_TARQEQ 652
#define NAIFID_AEGAEON 653

#define NAIFID_URANUS 799
#define NAIFID_ARIEL 701
#define NAIFID_UMBRIEL 702
#define NAIFID_TITANIA 703
#define NAIFID_OBERON 704
#define NAIFID_MIRANDA 705
#define NAIFID_CORDELIA 706
#define NAIFID_OPHELIA 707
#define NAIFID_BIANCA 708
#define NAIFID_CRESSIDA 709
#define NAIFID_DESDEMONA 710
#define NAIFID_JULIET 711
#define NAIFID_PORTIA 712
#define NAIFID_ROSALIND 713
#define NAIFID_BELINDA 714
#define NAIFID_PUCK 715
#define NAIFID_CALIBAN 716
#define NAIFID_SYCORAX 717
#define NAIFID_PROSPERO 718
#define NAIFID_SETEBOS 719
#define NAIFID_STEPHANO 720
#define NAIFID_TRINCULO 721
#define NAIFID_FRANCISCO 722
#define NAIFID_MARGARET 723
#define NAIFID_FERDINAND 724
#define NAIFID_PERDITA 725
#define NAIFID_MAB 726
#define NAIFID_CUPID 727

#define NAIFID_NEPTUNE 899
#define NAIFID_TRITON 801
#define NAIFID_NEREID 802
#define NAIFID_NAIAD 803
#define NAIFID_THALASSA 804
#define NAIFID_DESPINA 805
#define NAIFID_GALATEA 806
#define NAIFID_LARISSA 807
#define NAIFID_PROTEUS 808
#define NAIFID_HALIMEDE 809
#define NAIFID_PSAMATHE 810
#define NAIFID_SAO 811
#define NAIFID_LAOMEDEIA 812
#define NAIFID_NESO 813

#define NAIFID_PLUTO 999
#define NAIFID_CHARON 901
#define NAIFID_NIX 902
#define NAIFID_HYDRA 903
#define NAIFID_KERBEROS 904
#define NAIFID_STYX 905

/*! NAIF identification numbers for the comets (table 4 of reference 1)  */
#define NAIFID_AREND 1000001
#define NAIFID_AREND_RIGAUX 1000002
#define NAIFID_ASHBROOK_JACKSON 1000003
#define NAIFID_BOETHIN 1000004
#define NAIFID_BORRELLY 1000005
#define NAIFID_BOWELL_SKIFF 1000006
#define NAIFID_BRADFIELD 1000007
#define NAIFID_BROOKS_2 1000008
#define NAIFID_BRORSEN_METCALF 1000009
#define NAIFID_BUS 1000010
#define NAIFID_CHERNYKH 1000011
#define NAIFID_CHURYUMOV_GERASIMENKO 1000012
#define NAIFID_CIFFREO 1000013
#define NAIFID_CLARK 1000014
#define NAIFID_COMAS_SOLA 1000015
#define NAIFID_CROMMELIN 1000016
#define NAIFID_D__ARREST 1000017
#define NAIFID_DANIEL 1000018
#define NAIFID_DE_VICO_SWIFT 1000019
#define NAIFID_DENNING_FUJIKAWA 1000020
#define NAIFID_DU_TOIT_1 1000021
#define NAIFID_DU_TOIT_HARTLEY 1000022
#define NAIFID_DUTOIT_NEUJMIN_DELPORTE 1000023
#define NAIFID_DUBIAGO 1000024
#define NAIFID_ENCKE 1000025
#define NAIFID_FAYE 1000026
#define NAIFID_FINLAY 1000027
#define NAIFID_FORBES 1000028
#define NAIFID_GEHRELS_1 1000029
#define NAIFID_GEHRELS_2 1000030
#define NAIFID_GEHRELS_3 1000031
#define NAIFID_GIACOBINI_ZINNER 1000032
#define NAIFID_GICLAS 1000033
#define NAIFID_GRIGG_SKJELLERUP 1000034
#define NAIFID_GUNN 1000035
#define NAIFID_HALLEY 1000036
#define NAIFID_HANEDA_CAMPOS 1000037
#define NAIFID_HARRINGTON 1000038
#define NAIFID_HARRINGTON_ABELL 1000039
#define NAIFID_HARTLEY_1 1000040
#define NAIFID_HARTLEY_2 1000041
#define NAIFID_HARTLEY_IRAS 1000042
#define NAIFID_HERSCHEL_RIGOLLET 1000043
#define NAIFID_HOLMES 1000044
#define NAIFID_HONDA_MRKOS_PAJDUSAKOVA 1000045
#define NAIFID_HOWELL 1000046
#define NAIFID_IRAS 1000047
#define NAIFID_JACKSON_NEUJMIN 1000048
#define NAIFID_JOHNSON 1000049
#define NAIFID_KEARNS_KWEE 1000050
#define NAIFID_KLEMOLA 1000051
#define NAIFID_KOHOUTEK 1000052
#define NAIFID_KOJIMA 1000053
#define NAIFID_KOPFF 1000054
#define NAIFID_KOWAL_1 1000055
#define NAIFID_KOWAL_2 1000056
#define NAIFID_KOWAL_MRKOS 1000057
#define NAIFID_KOWAL_VAVROVA 1000058
#define NAIFID_LONGMORE 1000059
#define NAIFID_LOVAS_1 1000060
#define NAIFID_MACHHOLZ 1000061
#define NAIFID_MAURY 1000062
#define NAIFID_NEUJMIN_1 1000063
#define NAIFID_NEUJMIN_2 1000064
#define NAIFID_NEUJMIN_3 1000065
#define NAIFID_OLBERS 1000066
#define NAIFID_PETERS_HARTLEY 1000067
#define NAIFID_PONS_BROOKS 1000068
#define NAIFID_PONS_WINNECKE 1000069
#define NAIFID_REINMUTH_1 1000070
#define NAIFID_REINMUTH_2 1000071
#define NAIFID_RUSSELL_1 1000072
#define NAIFID_RUSSELL_2 1000073
#define NAIFID_RUSSELL_3 1000074
#define NAIFID_RUSSELL_4 1000075
#define NAIFID_SANGUIN 1000076
#define NAIFID_SCHAUMASSE 1000077
#define NAIFID_SCHUSTER 1000078
#define NAIFID_SCHWASSMANN_WACHMANN_1 1000079
#define NAIFID_SCHWASSMANN_WACHMANN_2 1000080
#define NAIFID_SCHWASSMANN_WACHMANN_3 1000081
#define NAIFID_SHAJN_SCHALDACH 1000082
#define NAIFID_SHOEMAKER_1 1000083
#define NAIFID_SHOEMAKER_2 1000084
#define NAIFID_SHOEMAKER_3 1000085
#define NAIFID_SINGER_BREWSTER 1000086
#define NAIFID_SLAUGHTER_BURNHAM 1000087
#define NAIFID_SMIRNOVA_CHERNYKH 1000088
#define NAIFID_STEPHAN_OTERMA 1000089
#define NAIFID_SWIFT_GEHRELS 1000090
#define NAIFID_TAKAMIZAWA 1000091
#define NAIFID_TAYLOR 1000092
#define NAIFID_TEMPEL_1 1000093
#define NAIFID_TEMPEL_2 1000094
#define NAIFID_TEMPEL_TUTTLE 1000095
#define NAIFID_TRITTON 1000096
#define NAIFID_TSUCHINSHAN_1 1000097
#define NAIFID_TSUCHINSHAN_2 1000098
#define NAIFID_TUTTLE 1000099
#define NAIFID_TUTTLE_GIACOBINI_KRESAK 1000100
#define NAIFID_VAISALA_1 1000101
#define NAIFID_VAN_BIESBROECK 1000102
#define NAIFID_VAN_HOUTEN 1000103
#define NAIFID_WEST_KOHOUTEK_IKEMURA 1000104
#define NAIFID_WHIPPLE 1000105
#define NAIFID_WILD_1 1000106
#define NAIFID_WILD_2 1000107
#define NAIFID_WILD_3 1000108
#define NAIFID_WIRTANEN 1000109
#define NAIFID_WOLF 1000110
#define NAIFID_WOLF_HARRINGTON 1000111
#define NAIFID_LOVAS_2 1000112
#define NAIFID_URATA_NIIJIMA 1000113
#define NAIFID_WISEMAN_SKIFF 1000114
#define NAIFID_HELIN 1000115
#define NAIFID_MUELLER 1000116
#define NAIFID_SHOEMAKER_HOLT_1 1000117
#define NAIFID_HELIN_ROMAN_CROCKETT 1000118
#define NAIFID_HARTLEY_3 1000119
#define NAIFID_PARKER_HARTLEY 1000120
#define NAIFID_HELIN_ROMAN_ALU_1 1000121
#define NAIFID_WILD_4 1000122
#define NAIFID_MUELLER_2 1000123
#define NAIFID_MUELLER_3 1000124
#define NAIFID_SHOEMAKER_LEVY_1 1000125
#define NAIFID_SHOEMAKER_LEVY_2 1000126
#define NAIFID_HOLT_OLMSTEAD 1000127
#define NAIFID_METCALF_BREWINGTON 1000128
#define NAIFID_LEVY 1000129
#define NAIFID_SHOEMAKER_LEVY_9 1000130
#define NAIFID_HYAKUTAKE 1000131
#define NAIFID_HALE_BOPP 1000132
#define NAIFID_SIDING_SPRING 1003228

  /*-----------------------------------------------------------------*/
  /* error handler */
  /*-----------------------------------------------------------------*/
  /*! set the error handler */
  __CALCEPH_DECLSPEC void calceph_seterrorhandler(int typehandler, void (*userfunc)(const char*));

  /*-----------------------------------------------------------------*/
  /* single access API per thread/process */
  /*-----------------------------------------------------------------*/
  /*! open an ephemeris data file */
  __CALCEPH_DECLSPEC int calceph_sopen(const char* filename);

  /*! return the version of the ephemeris data file as a null-terminated string */
  __CALCEPH_DECLSPEC int calceph_sgetfileversion(char szversion[CALCEPH_MAX_CONSTANTVALUE]);

  /*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
   for a given target and center */
  __CALCEPH_DECLSPEC int calceph_scompute(double JD0, double time, int target, int center, double PV[6]);

  /*! get the first value from the specified name constant in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_sgetconstant(const char* name, double* value);

  /*! return the number of constants available in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_sgetconstantcount(void);

  /*! return the name and the associated value of the constant available at some
   * index in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_sgetconstantindex(int index, char name[CALCEPH_MAX_CONSTANTNAME],
                                                   double* value);

  /*! return the time scale in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_sgettimescale(void);

  /*! return the first and last time available in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_sgettimespan(double* firsttime, double* lasttime, int* continuous);

  /*! close an ephemeris data file */
  __CALCEPH_DECLSPEC void calceph_sclose(void);

  /*-----------------------------------------------------------------*/
  /* multiple access API per thread/process */
  /*-----------------------------------------------------------------*/
  /*! ephemeris descriptor */
  typedef struct calcephbin t_calcephbin;

  /*! fixed length string value of a constant */
  typedef char t_calcephcharvalue[CALCEPH_MAX_CONSTANTVALUE];

  /*! open an ephemeris data file */
  __CALCEPH_DECLSPEC t_calcephbin* calceph_open(const char* filename);

  /*! open a list of ephemeris data file */
  __CALCEPH_DECLSPEC t_calcephbin* calceph_open_array(int n, const char* const filename[]);

#if 0
  /*! duplicate the handle of the ephemeris data file */
  __CALCEPH_DECLSPEC t_calcephbin* calceph_fdopen(t_calcephbin* eph);
#endif

  /*! return the version of the ephemeris data file as a null-terminated string */
  __CALCEPH_DECLSPEC int calceph_getfileversion(t_calcephbin* eph,
                                                char szversion[CALCEPH_MAX_CONSTANTVALUE]);

  /*! prefetch all data to memory */
  __CALCEPH_DECLSPEC int calceph_prefetch(t_calcephbin* eph);

  /*! return non-zero value if eph could be accessed by multiple threads */
  __CALCEPH_DECLSPEC int calceph_isthreadsafe(t_calcephbin* eph);

  /*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
   for a given target and center at a single time. The output is in UA, UA/day,
   radians */
  __CALCEPH_DECLSPEC int calceph_compute(t_calcephbin* eph, double JD0, double time, int target,
                                         int center, double PV[6]);

  /*! compute the position <x,y,z> and velocity <xdot,ydot,zdot>
   for a given target and center at a single time. The output is expressed
   according to unit */
  __CALCEPH_DECLSPEC int calceph_compute_unit(t_calcephbin* eph, double JD0, double time, int target,
                                              int center, int unit, double PV[6]);

  /*! compute the orientation <euler angles> and their derivatives for a given
   target  at a single time. The output is expressed according to unit */
  __CALCEPH_DECLSPEC int calceph_orient_unit(t_calcephbin* eph, double JD0, double time, int target,
                                             int unit, double PV[6]);

  /*! compute the rotational angular momentum G/(mR^2) and their derivatives for a
   given target  at a single time. The output is expressed according to unit */
  __CALCEPH_DECLSPEC int calceph_rotangmom_unit(t_calcephbin* eph, double JD0, double time, int target,
                                                int unit, double PV[6]);

  /*! According to the value of order, compute the position <x,y,z>
   and their first, second and third derivatives (velocity, acceleration, jerk)
   for a given target and center at a single time. The output is expressed
   according to unit */
  __CALCEPH_DECLSPEC int calceph_compute_order(t_calcephbin* eph, double JD0, double time, int target,
                                               int center, int unit, int order, double* PVAJ);

  /*! According to the value of order,  compute the orientation <euler angles> and
   their first, second and third derivatives for a given target  at a single time.
   The output is expressed according to unit */
  __CALCEPH_DECLSPEC int calceph_orient_order(t_calcephbin* eph, double JD0, double time, int target,
                                              int unit, int order, double* PVAJ);

  /*! compute the rotational angular momentum G/(mR^2) and their first, second and
   third derivatives for
   a given target at a single time. The output is expressed according to unit */
  __CALCEPH_DECLSPEC int calceph_rotangmom_order(t_calcephbin* eph, double JD0, double time, int target,
                                                 int unit, int order, double* PVAJ);

  /*! get the first value from the specified name constant in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getconstant(t_calcephbin* eph, const char* name, double* value);

  /*! get the first value from the specified name constant in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getconstantsd(t_calcephbin* eph, const char* name, double* value);

  /*! get the nvalue values from the specified name constant in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getconstantvd(t_calcephbin* eph, const char* name, double* arrayvalue,
                                               int nvalue);

  /*! get the first value from the specified name constant in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getconstantss(t_calcephbin* eph, const char* name,
                                               t_calcephcharvalue value);

  /*! get the nvalue values from the specified name constant in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getconstantvs(t_calcephbin* eph, const char* name,
                                               t_calcephcharvalue* arrayvalue, int nvalue);

  /*! return the number of constants available in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getconstantcount(t_calcephbin* eph);

  /*! return the name and the associated first value of the constant available at
   * some index in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getconstantindex(t_calcephbin* eph, int index,
                                                  char name[CALCEPH_MAX_CONSTANTNAME], double* value);

  /*! return the id of the body using the given name in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getidbyname(t_calcephbin* eph, const char* name, int unit, int* id);

  /*! return the first name of the body using its id in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getnamebyidss(t_calcephbin* eph, int id, int unit,
                                               t_calcephcharvalue value);

  /*! return the number of position’s records available in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getpositionrecordcount(t_calcephbin* eph);


  /*! return the time scale in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_gettimescale(t_calcephbin* eph);

  /*! return the first and last time available in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_gettimespan(t_calcephbin* eph, double* firsttime, double* lasttime,
                                             int* continuous);

  /*! return the number of position’s records available in the ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getpositionrecordcount(t_calcephbin* eph);

  /*! return the target and origin bodies, the first and last time, and the
   reference frame available at the specified position’s records' index of the
   ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getpositionrecordindex(t_calcephbin* eph, int index, int* target,
                                                        int* center, double* firsttime, double* lasttime,
                                                        int* frame);

  /*! return the target and origin bodies, the first and last time, the reference frame
   and the segment type available at the specified position’s records' index of the
   ephemeris file */
  __CALCEPH_DECLSPEC int calceph_getpositionrecordindex2(t_calcephbin* eph, int index, int* target,
                                                         int* center, double* firsttime,
                                                         double* lasttime, int* frame, int* segtype);

  /*! return the number of orientation’s records available in the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getorientrecordcount(t_calcephbin* eph);

  /*! return the target body, the first and last time, and the reference frame
   available at the specified orientation’s records' index of the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getorientrecordindex(t_calcephbin* eph, int index, int* target,
                                                      double* firsttime, double* lasttime, int* frame);

  /*! return the target body, the first and last time, the reference frame and the segment type
   available at the specified orientation’s records' index of the ephemeris file
   */
  __CALCEPH_DECLSPEC int calceph_getorientrecordindex2(t_calcephbin* eph, int index, int* target,
                                                       double* firsttime, double* lasttime, int* frame,
                                                       int* segtype);

  /*! close an ephemeris data file and destroy the ephemeris descriptor */
  __CALCEPH_DECLSPEC void calceph_close(t_calcephbin* eph);

  /*! return the maximal order of the derivatives for a segment type */
  __CALCEPH_DECLSPEC int calceph_getmaxsupportedorder(int idseg);

  /*! return the version of the library as a null-terminated string */
  __CALCEPH_DECLSPEC void calceph_getversion_str(char szversion[CALCEPH_MAX_CONSTANTNAME]);

/*----------------------------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif /*defined (__cplusplus) */

#endif /*__CALCEPH_H__*/
