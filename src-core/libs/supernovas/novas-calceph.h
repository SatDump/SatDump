/**
 * @file
 *
 * @date Created  on Nov 12, 2024
 * @author Attila Kovacs
 *
 *   SuperNOVAS functions interfacing with the CALCEPH C library.
 */

#ifndef NOVAS_CALCEPH_H_
#define NOVAS_CALCEPH_H_

#  include "libs/calceph/calceph.h"

int novas_use_calceph(t_calcephbin *eph);

int novas_use_calceph_planets(t_calcephbin *eph);

int novas_calceph_use_ids(enum novas_id_type idtype);


#endif /* NOVAS_CALCEPH_H_ */
