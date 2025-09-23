/**
 * @author Attila Kovacs
 * @version IERS Conventions v1.3.0
 *
 *  Earth orientation model data for variations in _x_<sub>p</sub> _y_<sub>p</sub> and
 *  DUT1 due to librations, with periods less than 2 days. (Longer period terms are
 *  included in the IAU2000A nutation model).
 */

/**
 * Polar migration due to librations. From IERS Conventions Chapter 5, Table 5.1a.
 *
 * REFERENCES:
 * <ol>
 * <li>https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * </ol>
 *
 * @since 1.5
 */
static const novas_eop_terms polar_libration_terms[10] = { //
        //                                         xp               yp
        //                                    sin     cos      sin      cos
        { {   1,  -1,   0,  -2,   0,  -1 },  -0.40,   0.30,  -0.30,  -0.40 }, //
        { {   1,  -1,   0,  -2,   0,  -2 },  -2.30,   1.30,  -1.30,  -2.30 }, //
        { {   1,   1,   0,  -2,  -2,  -2 },  -0.40,   0.30,  -0.30,  -0.40 }, //
        { {   1,   0,   0,  -2,   0,  -1 },  -2.10,   1.20,  -1.20,  -2.10 }, //
        { {   1,   0,   0,  -2,   0,  -2 }, -11.40,   6.50,  -6.50, -11.40 }, //
        { {   1,  -1,   0,   0,   0,   0 },   0.80,  -0.50,   0.50,   0.80 }, //
        { {   1,   0,   0,  -2,   2,  -2 },  -4.80,   2.70,  -2.70,  -4.80 }, //
        { {   1,   0,   0,   0,   0,   0 },  14.30,  -8.20,   8.20,  14.30 }, //
        { {   1,   0,   0,   0,   0,  -1 },   1.90,  -1.10,   1.10,   1.90 }, //
        { {   1,   1,   0,   0,   0,   0 },   0.80,  -0.40,   0.40,   0.80 }  //
};

/**
 * UT1 variations due to librations. From IERS Conventions Chapter 5, Table 5.1b.
 *
 * REFERENCES:
 * <ol>
 * <li>https://iers-conventions.obspm.fr/content/chapter5/icc5.pdf</li>
 * </ol>
 *
 * @since 1.5
 */
static const novas_eop_terms dut1_libration_terms[11] = {
        //                                         UT1             LOD
        //                                    sin     cos      sin     cos
        { {   2,  -2,   0,  -2,   0,  -2 },   0.05,  -0.03,  -0.30,  -0.60 }, //
        { {   2,   0,   0,  -2,  -2,  -2 },   0.06,  -0.03,  -0.40,  -0.70 }, //
        { {   2,  -1,   0,  -2,   0,  -2 },   0.35,  -0.20,  -2.40,  -4.20 }, //
        { {   2,   1,   0,  -2,  -2,  -2 },   0.07,  -0.04,  -0.50,  -0.80 }, //
        { {   2,   0,   0,  -2,   0,  -1 },  -0.07,   0.04,   0.50,   0.80 }, //
        { {   2,   0,   0,  -2,   0,  -2 },   1.75,  -1.01, -12.20, -21.30 }, //
        { {   2,   1,   0,  -2,   0,  -2 },  -0.05,   0.03,   0.30,   0.60 }, //
        { {   2,   0,  -1,  -2,   2,  -2 },   0.05,  -0.03,  -0.30,  -0.60 }, //
        { {   2,   0,   0,  -2,   2,  -2 },   0.76,  -0.44,  -5.50,  -9.50 }, //
        { {   2,   0,   0,   0,   0,   0 },   0.21,  -0.12,  -1.50,  -2.60 }, //
        { {   2,   0,   0,   0,   0,  -1 },   0.06,  -0.04,  -0.40,  -0.80 }  //
};
