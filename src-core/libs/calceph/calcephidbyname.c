/*-----------------------------------------------------------------*/
/*!
  \file calcephidbyname.c
  \brief find the id of a body using its name.

  \author  M. Gastineau
           Astronomie et Systemes Dynamiques, IMCCE, CNRS, Observatoire de Paris.

   Copyright, 2023-2024, CNRS
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
#include <ctype.h>

#include "calcephdebug.h"
#include "real.h"
#define __CALCEPH_WITHIN_CALCEPH 1
#include "calceph.h"
#include "calcephinternal.h"
#include "calcephspice.h"
#include "util.h"

/*! structure of the default lists  */
struct map_name_id
{
    const char *name;           /*!< name of the body */
    int id;                     /*!< id of the body */
};

/*! default list of objects using the INPOP/DE numbering system */
static struct map_name_id defaultlist_inpop[] = {
    {"MERCURY BARYCENTER", 1},
    {"VENUS BARYCENTER", 2},
    {"EARTH", 3},
    {"MARS BARYCENTER", 4},
    {"JUPITER BARYCENTER", 5},
    {"SATURN BARYCENTER", 6},
    {"URANUS BARYCENTER", 7},
    {"NEPTUNE BARYCENTER", 8},
    {"PLUTO BARYCENTER", 9},
    {"MOON", 10},
    {"SUN", 11},
    {"SOLAR SYSTEM BARYCENTER", 12},
    {"EARTH MOON BARYCENTER", 13},
};

/*! default list of objects using the NAIF numbering system */
static struct map_name_id defaultlist_naifid[] = { {"SOLAR SYSTEM BARYCENTER", 0},
{"MERCURY BARYCENTER", 1},
{"VENUS BARYCENTER", 2},
{"EARTH MOON BARYCENTER", 3},
{"MARS BARYCENTER", 4},
{"JUPITER BARYCENTER", 5},
{"SATURN BARYCENTER", 6},
{"URANUS BARYCENTER", 7},
{"NEPTUNE BARYCENTER", 8},
{"PLUTO BARYCENTER", 9},
{"SUN", 10},

{"MERCURY", 199},
{"VENUS", 299},

{"EARTH", 399},
{"MOON", 301},

{"MARS", 499},
{"PHOBOS", 401},
{"DEIMOS", 402},

{"JUPITER", 599},
{"IO", 501},
{"EUROPA", 502},
{"GANYMEDE", 503},
{"CALLISTO", 504},
{"AMALTHEA", 505},
{"HIMALIA", 506},
{"ELARA", 507},
{"PASIPHAE", 508},
{"SINOPE", 509},
{"LYSITHEA", 510},
{"CARME", 511},
{"ANANKE", 512},
{"LEDA", 513},
{"THEBE", 514},
{"ADRASTEA", 515},
{"METIS", 516},
{"CALLIRRHOE", 517},
{"THEMISTO", 518},
{"MEGACLITE", 519},
{"TAYGETE", 520},
{"CHALDENE", 521},
{"HARPALYKE", 522},
{"KALYKE", 523},
{"IOCASTE", 524},
{"ERINOME", 525},
{"ISONOE", 526},
{"PRAXIDIKE", 527},
{"AUTONOE", 528},
{"THYONE", 529},
{"HERMIPPE", 530},
{"AITNE", 531},
{"EURYDOME", 532},
{"EUANTHE", 533},
{"EUPORIE", 534},
{"ORTHOSIE", 535},
{"SPONDE", 536},
{"KALE", 537},
{"PASITHEE", 538},
{"HEGEMONE", 539},
{"MNEME", 540},
{"AOEDE", 541},
{"THELXINOE", 542},
{"ARCHE", 543},
{"KALLICHORE", 544},
{"HELIKE", 545},
{"CARPO", 546},
{"EUKELADE", 547},
{"CYLLENE", 548},
{"KORE", 549},
{"HERSE", 550},
{"DIA", 553},

{"SATURN", 699},
{"MIMAS", 601},
{"ENCELADUS", 602},
{"TETHYS", 603},
{"DIONE", 604},
{"RHEA", 605},
{"TITAN", 606},
{"HYPERION", 607},
{"IAPETUS", 608},
{"PHOEBE", 609},
{"JANUS", 610},
{"EPIMETHEUS", 611},
{"HELENE", 612},
{"TELESTO", 613},
{"CALYPSO", 614},
{"ATLAS", 615},
{"PROMETHEUS", 616},
{"PANDORA", 617},
{"PAN", 618},
{"YMIR", 619},
{"PAALIAQ", 620},
{"TARVOS", 621},
{"IJIRAQ", 622},
{"SUTTUNGR", 623},
{"KIVIUQ", 624},
{"MUNDILFARI", 625},
{"ALBIORIX", 626},
{"SKATHI", 627},
{"ERRIAPUS", 628},
{"SIARNAQ", 629},
{"THRYMR", 630},
{"NARVI", 631},
{"METHONE", 632},
{"PALLENE", 633},
{"POLYDEUCES", 634},
{"DAPHNIS", 635},
{"AEGIR", 636},
{"BEBHIONN", 637},
{"BERGELMIR", 638},
{"BESTLA", 639},
{"FARBAUTI", 640},
{"FENRIR", 641},
{"FORNJOT", 642},
{"HATI", 643},
{"HYROKKIN", 644},
{"KARI", 645},
{"LOGE", 646},
{"SKOLL", 647},
{"SURTUR", 648},
{"ANTHE", 649},
{"JARNSAXA", 650},
{"GREIP", 651},
{"TARQEQ", 652},
{"AEGAEON", 653},

{"URANUS", 799},
{"ARIEL", 701},
{"UMBRIEL", 702},
{"TITANIA", 703},
{"OBERON", 704},
{"MIRANDA", 705},
{"CORDELIA", 706},
{"OPHELIA", 707},
{"BIANCA", 708},
{"CRESSIDA", 709},
{"DESDEMONA", 710},
{"JULIET", 711},
{"PORTIA", 712},
{"ROSALIND", 713},
{"BELINDA", 714},
{"PUCK", 715},
{"CALIBAN", 716},
{"SYCORAX", 717},
{"PROSPERO", 718},
{"SETEBOS", 719},
{"STEPHANO", 720},
{"TRINCULO", 721},
{"FRANCISCO", 722},
{"MARGARET", 723},
{"FERDINAND", 724},
{"PERDITA", 725},
{"MAB", 726},
{"CUPID", 727},

{"NEPTUNE", 899},
{"TRITON", 801},
{"NEREID", 802},
{"NAIAD", 803},
{"THALASSA", 804},
{"DESPINA", 805},
{"GALATEA", 806},
{"LARISSA", 807},
{"PROTEUS", 808},
{"HALIMEDE", 809},
{"PSAMATHE", 810},
{"SAO", 811},
{"LAOMEDEIA", 812},
{"NESO", 813},

{"PLUTO", 999},
{"CHARON", 901},
{"NIX", 902},
{"HYDRA", 903},
{"KERBEROS", 904},
{"STYX", 905},

{"AREND", 1000001},
{"AREND-RIGAUX", 1000002},
{"ASHBROOK-JACKSON", 1000003},
{"BOETHIN", 1000004},
{"BORRELLY", 1000005},
{"BOWELL-SKIFF", 1000006},
{"BRADFIELD", 1000007},
{"BROOKS 2", 1000008},
{"BRORSEN-METCALF", 1000009},
{"BUS", 1000010},
{"CHERNYKH", 1000011},
{"CHURYUMOV-GERASIMENKO", 1000012},
{"CIFFREO", 1000013},
{"CLARK", 1000014},
{"COMAS SOLA", 1000015},
{"CROMMELIN", 1000016},
{"D'ARREST", 1000017},
{"DANIEL", 1000018},
{"DE VICO-SWIFT", 1000019},
{"DENNING-FUJIKAWA", 1000020},
{"DU TOIT 1", 1000021},
{"DU TOIT HARTLEY", 1000022},
{"DUTOIT-NEUJMIN-DELPORTE", 1000023},
{"DUBIAGO", 1000024},
{"ENCKE", 1000025},
{"FAYE", 1000026},
{"FINLAY", 1000027},
{"FORBES", 1000028},
{"GEHRELS 1", 1000029},
{"GEHRELS 2", 1000030},
{"GEHRELS 3", 1000031},
{"GIACOBINI-ZINNER", 1000032},
{"GICLAS", 1000033},
{"GRIGG-SKJELLERUP", 1000034},
{"GUNN", 1000035},
{"HALLEY", 1000036},
{"HANEDA-CAMPOS", 1000037},
{"HARRINGTON", 1000038},
{"HARRINGTON-ABELL", 1000039},
{"HARTLEY 1", 1000040},
{"HARTLEY 2", 1000041},
{"HARTLEY-IRAS", 1000042},
{"HERSCHEL-RIGOLLET", 1000043},
{"HOLMES", 1000044},
{"HONDA-MRKOS-PAJDUSAKOVA", 1000045},
{"HOWELL", 1000046},
{"IRAS", 1000047},
{"JACKSON-NEUJMIN", 1000048},
{"JOHNSON", 1000049},
{"KEARNS-KWEE", 1000050},
{"KLEMOLA", 1000051},
{"KOHOUTEK", 1000052},
{"KOJIMA", 1000053},
{"KOPFF", 1000054},
{"KOWAL 1", 1000055},
{"KOWAL 2", 1000056},
{"KOWAL-MRKOS", 1000057},
{"KOWAL-VAVROVA", 1000058},
{"LONGMORE", 1000059},
{"LOVAS 1", 1000060},
{"MACHHOLZ", 1000061},
{"MAURY", 1000062},
{"NEUJMIN 1", 1000063},
{"NEUJMIN 2", 1000064},
{"NEUJMIN 3", 1000065},
{"OLBERS", 1000066},
{"PETERS-HARTLEY", 1000067},
{"PONS-BROOKS", 1000068},
{"PONS-WINNECKE", 1000069},
{"REINMUTH 1", 1000070},
{"REINMUTH 2", 1000071},
{"RUSSELL 1", 1000072},
{"RUSSELL 2", 1000073},
{"RUSSELL 3", 1000074},
{"RUSSELL 4", 1000075},
{"SANGUIN", 1000076},
{"SCHAUMASSE", 1000077},
{"SCHUSTER", 1000078},
{"SCHWASSMANN-WACHMANN 1", 1000079},
{"SCHWASSMANN-WACHMANN 2", 1000080},
{"SCHWASSMANN-WACHMANN 3", 1000081},
{"SHAJN-SCHALDACH", 1000082},
{"SHOEMAKER 1", 1000083},
{"SHOEMAKER 2", 1000084},
{"SHOEMAKER 3", 1000085},
{"SINGER-BREWSTER", 1000086},
{"SLAUGHTER-BURNHAM", 1000087},
{"SMIRNOVA-CHERNYKH", 1000088},
{"STEPHAN-OTERMA", 1000089},
{"SWIFT-GEHRELS", 1000090},
{"TAKAMIZAWA", 1000091},
{"TAYLOR", 1000092},
{"TEMPEL 1", 1000093},
{"TEMPEL 2", 1000094},
{"TEMPEL-TUTTLE", 1000095},
{"TRITTON", 1000096},
{"TSUCHINSHAN 1", 1000097},
{"TSUCHINSHAN 2", 1000098},
{"TUTTLE", 1000099},
{"TUTTLE-GIACOBINI-KRESAK", 1000100},
{"VAISALA 1", 1000101},
{"VAN BIESBROECK", 1000102},
{"VAN HOUTEN", 1000103},
{"WEST-KOHOUTEK-IKEMURA", 1000104},
{"WHIPPLE", 1000105},
{"WILD 1", 1000106},
{"WILD 2", 1000107},
{"WILD 3", 1000108},
{"WIRTANEN", 1000109},
{"WOLF", 1000110},
{"WOLF-HARRINGTON", 1000111},
{"LOVAS 2", 1000112},
{"URATA-NIIJIMA", 1000113},
{"WISEMAN-SKIFF", 1000114},
{"HELIN", 1000115},
{"MUELLER", 1000116},
{"SHOEMAKER-HOLT 1", 1000117},
{"HELIN-ROMAN-CROCKETT", 1000118},
{"HARTLEY 3", 1000119},
{"PARKER-HARTLEY", 1000120},
{"HELIN-ROMAN-ALU 1", 1000121},
{"WILD 4", 1000122},
{"MUELLER 2", 1000123},
{"MUELLER 3", 1000124},
{"SHOEMAKER-LEVY 1", 1000125},
{"SHOEMAKER-LEVY 2", 1000126},
{"HOLT-OLMSTEAD", 1000127},
{"METCALF-BREWINGTON", 1000128},
{"LEVY", 1000129},
{"SHOEMAKER-LEVY 9", 1000130},
{"HYAKUTAKE", 1000131},
{"HALE-BOPP", 1000132},
{"C/2013 A1", 1003228},
{"SIDING SPRING", 1003228}
};

/*--------------------------------------------------------------------------*/
/*! remove leading and trailing space and multiple spaces between words
   return 0 if failure.
   return 1 on sucess.

 @param name (in) uncompressed name
 @param compressed_name (in) compressed name
*/
/*--------------------------------------------------------------------------*/
static int calceph_compress_name(const t_calcephcharvalue name, t_calcephcharvalue compressed_name)
{
    int isrc = 0;
    int idst = 0;
    int prevspace = 1;          /* =0 if the previous character is a space */

    for (; isrc < CALCEPH_MAX_CONSTANTVALUE && name[isrc] != '\0'; isrc++)
    {
        if (!isspace_char(name[isrc]))
        {
            compressed_name[idst++] = name[isrc];
            prevspace = 0;
        }
        else
        {                       /* name[isrc] is a space character */
            if (prevspace == 0)
                compressed_name[idst++] = name[isrc];
            prevspace = 1;
        }
    }
    if (prevspace == 1 && idst > 0)
        idst--;
    compressed_name[idst] = '\0';
    return (idst != 0) ? 1 : 0;
}

/*--------------------------------------------------------------------------*/
/*! store, in id, the identification number of the body, given its name.
   return 0 if the name was not found.
   return 1 on sucess.

  @param list (in) array of the tuple (name, id)
  @param len_list (in) number of the elements in the array list
  @param name (in) null-terminated string of the name (without leading, trailing and multiple spaces)
  @param id (out) identification number.
     The value depends on the numbering system given by value.

*/
/*--------------------------------------------------------------------------*/
static int calceph_findbyname_in_defaultlist(const struct map_name_id *list, size_t len_list,
                                             t_calcephcharvalue name, int *id)
{
    size_t k;

    for (k = 0; k < len_list; k++)
    {
        if (strcasecmp(list[k].name, name) == 0)
        {
            *id = list[k].id;
            return 1;
        }
    }

    return 0;
}

/*--------------------------------------------------------------------------*/
/*! store, in id, the identification number of the body, given its name.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (in) null-terminated string of the name
  @param id (out) identification number using NAIFID numbering system

*/
/*--------------------------------------------------------------------------*/
static int calceph_spicekernel_findbyname_in_txtpck(struct SPICEkernel *eph, const t_calcephcharvalue name, int *id)
{
    int found = 0;

    const struct TXTPCKconstant *ptr_name = calceph_spicekernel_getptrconstant(eph, "NAIF_BODY_NAME");
    const struct TXTPCKconstant *ptr_code = calceph_spicekernel_getptrconstant(eph, "NAIF_BODY_CODE");

    if (ptr_name != NULL && ptr_code != NULL)
    {
        struct TXTPCKvalue *listvalue_name = ptr_name->value;
        struct TXTPCKvalue *listvalue_code = ptr_code->value;

        for (; listvalue_name != NULL && listvalue_code != NULL && found == 0;
             listvalue_name = listvalue_name->next, listvalue_code = listvalue_code->next)
        {
            t_calcephcharvalue value;
            int validname = 0;

            if (listvalue_name->buffer[listvalue_name->locfirst] == '\'')
            {
                int k;

                int j = listvalue_name->loclast;

                while (j > listvalue_name->locfirst && listvalue_name->buffer[j] != '\'')
                    j--;
                if (j > listvalue_name->locfirst)
                {
                    int curpos = 0;

                    for (k = listvalue_name->locfirst + 1; k < j && curpos < CALCEPH_MAX_CONSTANTVALUE; k++)
                    {
                        if (listvalue_name->buffer[k] == '\'')
                            k++;
                        value[curpos++] = listvalue_name->buffer[k];
                    }
                    value[curpos++] = '\0';
                    validname = 1;
                }
            }

            if (validname && listvalue_code->buffer[listvalue_code->locfirst] != '\'')
            {
                if (strcasecmp(value, name) == 0)
                {
                    char *perr;
                    long lid = strtol(listvalue_code->buffer + listvalue_code->locfirst, &perr, 10);

                    found = (((off_t) (perr - listvalue_code->buffer)) <= listvalue_code->loclast) ? 1 : 0;
                    *id = (int) lid;
                }
            }
        }
    }

    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in id, the identification number of the body, given its name.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (in) null-terminated string of the name
  @param id (out) identification number using NAIFID numbering system
*/
/*--------------------------------------------------------------------------*/
static int calceph_spice_findbyname_in_txtpck(const struct calcephbin_spice *eph,
                                              const t_calcephcharvalue name, int *id)
{
    int found = 0;

    struct SPICEkernel *list = eph->list;

    while (list != NULL && found == 0)
    {
        found = calceph_spicekernel_findbyname_in_txtpck(list, name, id);
        list = list->next;
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in id, the identification number of the body, given its name.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (in) null-terminated string of the name
  @param id (out) identification number using NAIFID numbering system
*/
/*--------------------------------------------------------------------------*/
static int calceph_findbyname_in_txtpck(t_calcephbin * eph, const t_calcephcharvalue name, int *id)
{
    int found = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            found = calceph_spice_findbyname_in_txtpck(&eph->data.spkernel, name, id);
            break;

        case CALCEPH_ebinary:

            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_getidbyname\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in id, the identification number of the body, given its name.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (in) null-terminated string of the name
  @param unit (in) 0 or CALCEPH_USE_NAIFID
  @param id (out) identification number.
     The value depends on the numbering system given by value.

*/
/*--------------------------------------------------------------------------*/
int calceph_getidbyname(t_calcephbin * eph, const char *name, int unit, int *id)
{
    t_calcephcharvalue compressed_name;
    int found = 0;

    if (calceph_compress_name(name, compressed_name) == 0)
        return 0;

    /* search in the JPL/INPOP list  */
    if ((unit & CALCEPH_USE_NAIFID) == 0)
    {
        found = calceph_findbyname_in_defaultlist(defaultlist_inpop,
                                                  sizeof(defaultlist_inpop) / sizeof(struct map_name_id),
                                                  compressed_name, id);
    }
    /* search in the list if the spice numbering system is used */
    else
    {
        found = calceph_findbyname_in_txtpck(eph, compressed_name, id);
        if (found == 0)
        {
            found = calceph_findbyname_in_defaultlist(defaultlist_naifid,
                                                      sizeof(defaultlist_naifid) / sizeof(struct map_name_id),
                                                      compressed_name, id);
        }
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in name, the name of the body, given its identification number.
   return 0 if the name was not found.
   return 1 on sucess.

  @param list (in) array of the tuple (name, id)
  @param len_list (in) number of the elements in the array list
  @param name (out) null-terminated string of the name
  @param id (in) identification number.
     The value depends on the numbering system given by value.
*/
/*--------------------------------------------------------------------------*/
static int calceph_findbyid_in_defaultlist(const struct map_name_id *list, size_t len_list, int id,
                                           t_calcephcharvalue name)
{
    size_t k;

    for (k = 0; k < len_list; k++)
    {
        if (id == list[k].id)
        {
            strncpy(name, list[k].name, CALCEPH_MAX_CONSTANTVALUE-1);
            name[CALCEPH_MAX_CONSTANTVALUE-1] = '\0';
            return 1;
        }
    }

    return 0;
}

/*--------------------------------------------------------------------------*/
/*! store, in name, the name of the body, given its identification number.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (out) null-terminated string of the name
  @param id (in) identification number using NAIFID numbering system

*/
/*--------------------------------------------------------------------------*/
static int calceph_spicekernel_findbyid_in_txtpck(struct SPICEkernel *eph, int id, t_calcephcharvalue name)
{
    int found = 0;
    char sid[20];

    const struct TXTPCKconstant *ptr_name = calceph_spicekernel_getptrconstant(eph, "NAIF_BODY_NAME");
    const struct TXTPCKconstant *ptr_code = calceph_spicekernel_getptrconstant(eph, "NAIF_BODY_CODE");

    calceph_snprintf(sid, 20, "%d", id);

    if (ptr_name != NULL && ptr_code != NULL)
    {
        struct TXTPCKvalue *listvalue_name = ptr_name->value;
        struct TXTPCKvalue *listvalue_code = ptr_code->value;

        for (; listvalue_name != NULL && listvalue_code != NULL && found == 0;
             listvalue_name = listvalue_name->next, listvalue_code = listvalue_code->next)
        {
            int validid = 0;

            if (listvalue_code->buffer[listvalue_code->locfirst] != '\'')
            {
                size_t k = 0;

                size_t len_sid = strlen(sid);

                int j;

                validid = 1;
                for (j = listvalue_code->locfirst; j <= listvalue_code->loclast && k < len_sid && validid == 1;
                     j++, k++)
                {
                    if (sid[k] != listvalue_code->buffer[j])
                        validid = 0;
                }
            }

            if (validid && listvalue_name->buffer[listvalue_name->locfirst] == '\'')
            {
                int j = 0;

                while (listvalue_name->locfirst + j + 1 < listvalue_name->loclast
                       && listvalue_name->buffer[listvalue_name->locfirst + j + 1] != '\'')
                {
                    name[j] = listvalue_name->buffer[listvalue_name->locfirst + j + 1];
                    j++;
                }
                name[j] = '\0';
                found = 1;
            }
        }
    }

    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in name, the name of the body, given its identification number.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (out) null-terminated string of the name
  @param id (in) identification number using NAIFID numbering system
*/
/*--------------------------------------------------------------------------*/
static int calceph_spice_findbyid_in_txtpck(const struct calcephbin_spice *eph, int id, t_calcephcharvalue name)
{
    int found = 0;

    struct SPICEkernel *list = eph->list;

    while (list != NULL && found == 0)
    {
        found = calceph_spicekernel_findbyid_in_txtpck(list, id, name);
        list = list->next;
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in name, the name of the body, given its identification number.
   return 0 if the name was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param name (out) null-terminated string of the name
  @param id (in) identification number using NAIFID numbering system
*/
/*--------------------------------------------------------------------------*/
static int calceph_findbyid_in_txtpck(t_calcephbin * eph, int id, t_calcephcharvalue name)
{
    int found = 0;

    switch (eph->etype)
    {
        case CALCEPH_espice:
            found = calceph_spice_findbyid_in_txtpck(&eph->data.spkernel, id, name);
            break;

        case CALCEPH_ebinary:

            break;

        default:
            /* GCOVR_EXCL_START */
            fatalerror("Unknown ephemeris type in calceph_getnamebyid\n");
            /* GCOVR_EXCL_STOP */
            break;
    }
    return found;
}

/*--------------------------------------------------------------------------*/
/*! store, in value, the name of the body, given its identification number.
   return 0 if the identification number was not found.
   return 1 on sucess.

  @param eph (inout) ephemeris descriptor
  @param value (out) null-terminated string of the name
  @param unit (in) 0 or CALCEPH_USE_NAIFID
  @param id (in) identification number.
     The value depends on the numbering system given by value.

*/
/*--------------------------------------------------------------------------*/
__CALCEPH_DECLSPEC int calceph_getnamebyidss(t_calcephbin * eph, int id, int unit, t_calcephcharvalue value)
{
    int k;
    int found = 0;

    for (k = 0; k < CALCEPH_MAX_CONSTANTVALUE; k++)
        value[k] = ' ';
    /* search in the JPL/INPOP list  */
    if ((unit & CALCEPH_USE_NAIFID) == 0)
    {
        found = calceph_findbyid_in_defaultlist(defaultlist_inpop,
                                                sizeof(defaultlist_inpop) / sizeof(struct map_name_id), id, value);
    }
    /* search in the list if the spice numbering system is used */
    else
    {
        found = calceph_findbyid_in_txtpck(eph, id, value);
        if (found == 0)
        {
            found = calceph_findbyid_in_defaultlist(defaultlist_naifid,
                                                    sizeof(defaultlist_naifid) / sizeof(struct map_name_id), id, value);
        }
    }
    return found;
}
