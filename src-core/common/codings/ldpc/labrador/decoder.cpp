#include "decoder.h"
#include "codes/parity.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace satdump
{
    namespace labrador
    {
        template <typename T>
        inline T t_one()
        {
            return 1.0;
        }

        template <typename T>
        inline T t_zero()
        {
            return 0.0;
        }

        template <typename T>
        inline T t_maxval()
        {
            return std::numeric_limits<T>::max();
        }

        template <typename T>
        inline bool hard_bit(T v)
        {
            return v < 0.0;
        }

        template <typename T>
        T saturating_add(T x, T y)
        {
            int32_t sum = static_cast<int32_t>(x) + static_cast<int32_t>(y);

            if (sum > std::numeric_limits<T>::max())
                return std::numeric_limits<T>::max(); // 127
            else if (sum < std::numeric_limits<T>::min())
                return std::numeric_limits<T>::min(); // -128

            else
                return static_cast<T>(sum);
        }

        template <typename T>
        T saturating_sub(T x, T y)
        {
            int32_t sum = static_cast<int32_t>(x) - static_cast<int32_t>(y);

            if (sum > std::numeric_limits<T>::max())
                return std::numeric_limits<T>::max(); // 127
            else if (sum < std::numeric_limits<T>::min())
                return std::numeric_limits<T>::min(); // -128

            else
                return static_cast<T>(sum);
        }

        template <typename T>
        bool decode_ms(code_params_t c, T *llrs, uint8_t *output, T *working, uint8_t *working_u8, uint64_t maxiters, uint64_t *fiters)
        {
            const uint64_t &n = c.n;
            const uint64_t &k = c.k;
            const uint64_t &p = c.punctured_bits;

            // Check node Iterators
            parity_iter_t e;
            uint64_t check, var;

            // assert_eq!(llrs.len(), n, "llrs.len() != n");
            // assert_eq!(output.len(), self.output_len(), "output.len() != (n+p)/8");
            // assert_eq!(working.len(), self.decode_ms_working_len(), "working.len() incorrect");
            // assert_eq!(working_u8.len(), self.decode_ms_working_u8_len(), "working_u8 != (n+p-k)/8");

            // Rename output to parities as we'll use it to keep track of the status of each check's
            // parity equations, until using it to write the decoded output on completion.
            uint8_t *parities = output;

            // Rename working_u8 to ui_sgns.
            // It will be used to accumulate products of signs of each check's incoming messages.
            uint8_t *ui_sgns = working_u8;
            for (uint64_t i = 0; i < c.decode_ms_working_u8_len; i++)
                ui_sgns[i] = 0;

            // Split up the working area.
            // `u` contains check-to-variable messages, `v` contains variable-to-check messages,
            // `va` contains the marginal a posteriori LLRs for each variable, `ui_min*` tracks the
            // smallest and second-smallest variable-to-check message for each check.
            for (uint64_t i = 0; i < c.decode_ms_working_len; i++)
                working[i] = t_zero<T>();
            T *u = &working[0];
            T *v = &working[c.paritycheck_sum];
            T *va = &working[c.paritycheck_sum + c.paritycheck_sum];
            T *ui_min1 = &working[c.paritycheck_sum + c.paritycheck_sum + (n + p)];
            T *ui_min2 = &working[c.paritycheck_sum + c.paritycheck_sum + (n + p) + (n + p - k)];

            for (uint64_t iter = 0; iter < maxiters; iter++)
            {
                // Initialise the marginals to the input LLRs (and to 0 for punctured bits).
                memcpy(va, llrs, n * sizeof(T));
                for (uint64_t i = n; i < n + p; i++)
                    va[i] = t_zero<T>();

                // You'd think .enumerate() would be sensible, but actually it prevents
                // inlining the iterator's next() method, which leads to a big performance hit.
                uint64_t idx = 0;
                parity_iter_init(&e, c.c);
                while (parity_iter_next(&e, &check, &var))
                {
                    // If this variable-to-check message equals the minimum of all messages to this
                    // check, use the second-minimum to obtain the minimum-excluding-this-variable.
                    if (abs(v[idx]) == ui_min1[check])
                        u[idx] = ui_min2[check];
                    else
                        u[idx] = ui_min1[check];

                    // When the product of all incoming signs was -1, negate `u`.
                    if (((ui_sgns[check / 8] >> (check % 8)) & 1) == 1)
                        u[idx] = -u[idx];

                    // Remove the effect of the sign of this variable's message to this check node.
                    if (hard_bit(v[idx]))
                        u[idx] = -u[idx];

                    // Accumulate with all incoming messages to this variable.
                    va[var] = saturating_add(va[var], u[idx]);

                    idx += 1;
                }

                // Compute variable-to-check messages `v`.
                for (uint64_t i = 0; i < n + p - k; i++)
                    ui_min1[i] = t_maxval<T>();
                for (uint64_t i = 0; i < n + p - k; i++)
                    ui_min2[i] = t_maxval<T>();
                for (uint64_t i = 0; i < c.decode_ms_working_u8_len; i++)
                    ui_sgns[i] = 0;
                for (uint64_t i = 0; i < c.output_len; i++)
                    parities[i] = 0;
                idx = 0;

                parity_iter_init(&e, c.c);
                while (parity_iter_next(&e, &check, &var))
                {
                    // Compute new `v`, with self-correcting behaviour.
                    T new_v_ai = saturating_sub(va[var], u[idx]);
                    if (hard_bit(new_v_ai) == hard_bit(v[idx]) || v[idx] == t_zero<T>())
                        v[idx] = new_v_ai;
                    else
                        v[idx] = t_zero<T>();

                    // Track the smallest and second-smallest abs(variable-to-check) messages.
                    // These may be the same if two messages have identical magnitude.
                    if (abs(v[idx]) < ui_min1[check])
                    {
                        ui_min2[check] = ui_min1[check];
                        ui_min1[check] = abs(v[idx]);
                    }
                    else if (abs(v[idx]) < ui_min2[check])
                        ui_min2[check] = abs(v[idx]);

                    // Accumulate product of signs of variable-to-check messages, with a set
                    // bit meaning the product is negative.
                    if (hard_bit(v[idx]))
                        ui_sgns[check / 8] ^= 1 << (check % 8);

                    // Accumulate output of each check's parity equation, used to detect
                    // when all parity checks are satisfied and thus decoding is complete.
                    if (hard_bit(va[var]))
                        parities[check / 8] ^= 1 << (check % 8);

                    idx += 1;
                }

                // Check parity equations. If none are 1 then we have a valid codeword.
                int max = 0;
                for (uint64_t i = 0; i < (n + p) / 8; i++)
                    max |= parities[i];

                if (max == 0)
                {
                    output = parities;
                    for (uint64_t i = 0; i < (n + p) / 8; i++)
                        output[i] = 0;
                    for (uint64_t var = 0; var < n + p; var++)
                    {
                        T &v = va[var];
                        if (hard_bit(v))
                            output[var / 8] |= 1 << (7 - (var % 8));
                    }

                    *fiters = iter;
                    return true;
                }
            }

            // If we failed to find a codeword, at least hard decode the marginals into the output.
            output = parities;
            for (uint64_t i = 0; i < (n + p) / 8; i++)
                output[i] = 0;
            for (uint64_t var = 0; var < n + p; var++)
            {
                T &v = va[var];
                if (hard_bit(v))
                    output[var / 8] |= 1 << (7 - (var % 8));
            }

            *fiters = maxiters;
            return false;
        }

        template bool decode_ms<int8_t>(code_params_t, int8_t *, uint8_t *output, int8_t *working, uint8_t *working_u8, uint64_t maxiters, uint64_t *fiters);
        template bool decode_ms<int16_t>(code_params_t, int16_t *, uint8_t *output, int16_t *working, uint8_t *working_u8, uint64_t maxiters, uint64_t *fiters);
    } // namespace labrador
} // namespace satdump