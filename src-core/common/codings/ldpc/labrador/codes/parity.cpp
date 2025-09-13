#include "parity.h"
#include "codes.h"
#include "compact_parity_checks.h"
#include <cstdint>
#include <cstring>

namespace satdump
{
    namespace labrador
    {
        uint64_t trailing_zeros(uint64_t n)
        {
            unsigned bits = 0, x = n;
            if (x)
            {
                while ((x & 1) == 0)
                {
                    ++bits;
                    x >>= 1;
                }
            }
            return bits;
        }

        void parity_iter_init(parity_iter_t *i, ldpc_code_t c)
        {
            code_params_t p = get_code_params(c);

            int m = p.submatrix_size;
            int subm = (*p.ptr_compact_h)[0][0][0]; // First H matrix element

            i->phi = p.ptr_phi_j_k;
            i->prototype = p.ptr_compact_h;

            i->m = m;
            i->logmd4 = trailing_zeros(m / 4);
            i->modm = m - 1;
            i->modmd4 = (m / 4) - 1;
            i->rowidx = 0;
            i->colidx = 0;
            i->sub_mat_idx = 0;
            i->check = 0;
            i->sub_mat = subm;
            i->sub_mat_val = (subm & 0x3F);
        }

        bool parity_iter_next(parity_iter_t *i, uint64_t *check, uint64_t *var)
        {
            while (1) // Loop over rows of the prototype
            {
                while (1) // Loop over columns of the prototype
                {
                    while (1) // Loop over the three sub-prototypes we have to sum for each cell of the prototype
                    {
                        // If we have not yet yielded enough edges for this sub_mat
                        if (i->check < i->m)
                        {
                            // Weirdly doing this & operation every loop is faster than doing it just
                            // when we update self.sub_mat. Presumably the hint helps it match.
                            if ((i->sub_mat & (HP | HI)) == HI)
                            {
                                *check = i->rowidx * i->m + i->check;
                                *var = i->colidx * i->m + ((i->check + i->sub_mat_val) & i->modm);
                                i->check += 1;
                                return true;
                            }
                            else if ((i->sub_mat & (HP | HI)) == HP)
                            {
                                auto pi = ((((uint64_t)THETA_K[i->sub_mat_val] + (i->check >> i->logmd4)) % 4) << i->logmd4) +
                                          (((uint64_t)(*i->phi)[i->check >> i->logmd4][i->sub_mat_val] + i->check) & i->modmd4);
                                *check = i->rowidx * i->m + i->check;
                                *var = i->colidx * i->m + pi;
                                i->check += 1;
                                return true;
                            }
                        }

                        // Once we're done yielding results for this cell, reset check to 0.
                        i->check = 0;

                        // Advance which of the three sub-matrices we're summing.
                        // If sub_mat is 0, there won't be any new ones to sum, so stop then too.
                        if (i->sub_mat != 0 && i->sub_mat_idx < 2)
                        {
                            i->sub_mat_idx += 1;
                            i->sub_mat = (*i->prototype)[i->sub_mat_idx][i->rowidx][i->colidx];
                            i->sub_mat_val = uint64_t(i->sub_mat & 0x3F);
                        }
                        else
                        {
                            i->sub_mat_idx = 0;
                            break;
                        }
                    }

                    // Advance colidx. The number of active columns depends on the prototype.
                    if (i->colidx < 10)
                    {
                        i->colidx += 1;
                        i->sub_mat = (*i->prototype)[i->sub_mat_idx][i->rowidx][i->colidx];
                        i->sub_mat_val = uint64_t(i->sub_mat & 0x3F);
                    }
                    else
                    {
                        i->colidx = 0;
                        break;
                    }
                }

                // Advance rowidx. The number of rows depends on the prototype.
                if (i->rowidx < 3)
                {
                    i->rowidx += 1;
                    i->sub_mat = (*i->prototype)[i->sub_mat_idx][i->rowidx][i->colidx];
                    i->sub_mat_val = uint64_t(i->sub_mat & 0x3F);
                }
                else
                {
                    return false;
                }
            }
        }
    } // namespace labrador
} // namespace satdump