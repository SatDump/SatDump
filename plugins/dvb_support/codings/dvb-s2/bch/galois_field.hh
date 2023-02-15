/* -*- c++ -*- */
/*
 * Copyright 2018 Ahmet Inan, Ron Economos.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#include <cassert>

namespace dvbs2
{
    namespace CODE
    {
        namespace GF
        {

            template <int M, int POLY, typename TYPE>
            struct Index;

            template <int M, int POLY, typename TYPE>
            struct Value
            {
                static const int Q = 1 << M, N = Q - 1;
                // static_assert(M <= 8 * sizeof(TYPE), "TYPE not wide enough");
                // static_assert(Q == (POLY & ~N), "POLY not of degree Q");
                TYPE v;
                Value()
                {
                }
                explicit Value(TYPE v) : v(v)
                {
                    //    assert(v <= N);
                }
                explicit operator bool() const
                {
                    return v;
                }
                explicit operator int() const
                {
                    return v;
                }
                Value<M, POLY, TYPE>
                operator*=(Index<M, POLY, TYPE> a)
                {
                    //    assert(a.i < a.modulus());
                    return *this = *this * a;
                }
                Value<M, POLY, TYPE>
                operator*=(Value<M, POLY, TYPE> a)
                {
                    //    assert(a.v <= a.N);
                    return *this = *this * a;
                }
                Value<M, POLY, TYPE>
                operator+=(Value<M, POLY, TYPE> a)
                {
                    //    assert(a.v <= a.N);
                    return *this = *this + a;
                }
                static const Value<M, POLY, TYPE>
                zero()
                {
                    return Value<M, POLY, TYPE>(0);
                }
            };

            template <int M, int POLY, typename TYPE>
            struct Index
            {
                static const int Q = 1 << M, N = Q - 1;
                // static_assert(M <= 8 * sizeof(TYPE), "TYPE not wide enough");
                // static_assert(Q == (POLY & ~N), "POLY not of degree Q");
                TYPE i;
                Index()
                {
                }
                explicit Index(TYPE i) : i(i)
                {
                    //    assert(i < modulus());
                }
                explicit operator int() const
                {
                    return i;
                }
                Index<M, POLY, TYPE>
                operator*=(Index<M, POLY, TYPE> a)
                {
                    //    assert(a.i < a.modulus());
                    //    assert(i < modulus());
                    return *this = *this * a;
                }
                Index<M, POLY, TYPE>
                operator/=(Index<M, POLY, TYPE> a)
                {
                    //    assert(a.i < a.modulus());
                    //    assert(i < modulus());
                    return *this = *this / a;
                }
                static const TYPE
                modulus()
                {
                    return N;
                }
            };

            template <int M, int POLY, typename TYPE>
            struct Tables
            {
                static const int Q = 1 << M, N = Q - 1;
                // static_assert(M <= 8 * sizeof(TYPE), "TYPE not wide enough");
                // static_assert(Q == (POLY & ~N), "POLY not of degree Q");
                static TYPE *LOG, *EXP;
                TYPE log_[Q], exp_[Q];
                static TYPE
                next(TYPE a)
                {
                    return a & (TYPE)(Q >> 1) ? (a << 1) ^ (TYPE)POLY : a << 1;
                }
                static TYPE
                log(TYPE a)
                {
                    //    assert(LOG != nullptr);
                    //    assert(a <= N);
                    return LOG[a];
                }
                static TYPE
                exp(TYPE a)
                {
                    //    assert(EXP != nullptr);
                    //    assert(a <= N);
                    return EXP[a];
                }
                Tables()
                {
                    //    assert(LOG == nullptr);
                    LOG = log_;
                    //    assert(EXP == nullptr);
                    EXP = exp_;
                    log_[exp_[N] = 0] = N;
                    TYPE a = 1;
                    for (int i = 0; i < N; ++i, a = next(a))
                    {
                        log_[exp_[i] = a] = i;
                        //    assert(!i || a != 1);
                    }
                    //    assert(1 == a);
                }
                ~Tables()
                {
                    //    assert(LOG != nullptr);
                    LOG = nullptr;
                    //    assert(EXP != nullptr);
                    EXP = nullptr;
                }
            };

            template <int M, int POLY, typename TYPE>
            TYPE *Tables<M, POLY, TYPE>::LOG;

            template <int M, int POLY, typename TYPE>
            TYPE *Tables<M, POLY, TYPE>::EXP;

            template <int M, int POLY, typename TYPE>
            Index<M, POLY, TYPE>
            index(Value<M, POLY, TYPE> a)
            {
                //    assert(a.v <= a.N);
                //    assert(a.v);
                return Index<M, POLY, TYPE>(Tables<M, POLY, TYPE>::log(a.v));
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            value(Index<M, POLY, TYPE> a)
            {
                //    assert(a.i < a.modulus());
                return Value<M, POLY, TYPE>(Tables<M, POLY, TYPE>::exp(a.i));
            }

            template <int M, int POLY, typename TYPE>
            bool
            operator==(Value<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.v <= b.N);
                return a.v == b.v;
            }

            template <int M, int POLY, typename TYPE>
            bool
            operator!=(Value<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.v <= b.N);
                return a.v != b.v;
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator+(Value<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.v <= b.N);
                return Value<M, POLY, TYPE>(a.v ^ b.v);
            }

            template <int M, int POLY, typename TYPE>
            Index<M, POLY, TYPE>
            operator*(Index<M, POLY, TYPE> a, Index<M, POLY, TYPE> b)
            {
                //    assert(a.i < a.modulus());
                //    assert(b.i < b.modulus());
                TYPE tmp = a.i + b.i;
                return Index<M, POLY, TYPE>(a.modulus() - a.i <= b.i ? tmp - a.modulus() : tmp);
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator*(Value<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.v <= b.N);
                return (!a.v || !b.v) ? a.zero() : value(index(a) * index(b));
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            rcp(Value<M, POLY, TYPE> a)
            {
                //    assert(a.v <= a.N);
                //    assert(a.v);
                return value(Index<M, POLY, TYPE>(index(a).modulus() - index(a).i));
            }

            template <int M, int POLY, typename TYPE>
            Index<M, POLY, TYPE>
            operator/(Index<M, POLY, TYPE> a, Index<M, POLY, TYPE> b)
            {
                //    assert(a.i < a.modulus());
                //    assert(b.i < b.modulus());
                TYPE tmp = a.i - b.i;
                return Index<M, POLY, TYPE>(a.i < b.i ? tmp + a.modulus() : tmp);
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator/(Value<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.v <= b.N);
                //    assert(b.v);
                return !a.v ? a.zero() : value(index(a) / index(b));
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator/(Index<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.i < a.modulus());
                //    assert(b.v <= b.N);
                //    assert(b.v);
                return value(a / index(b));
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator/(Value<M, POLY, TYPE> a, Index<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.i < b.modulus());
                return !a.v ? a.zero() : value(index(a) / b);
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator*(Index<M, POLY, TYPE> a, Value<M, POLY, TYPE> b)
            {
                //    assert(a.i < a.modulus());
                //    assert(b.v <= b.N);
                return !b.v ? b.zero() : value(a * index(b));
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            operator*(Value<M, POLY, TYPE> a, Index<M, POLY, TYPE> b)
            {
                //    assert(a.v <= a.N);
                //    assert(b.i < b.modulus());
                return !a.v ? a.zero() : value(index(a) * b);
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            fma(Index<M, POLY, TYPE> a, Index<M, POLY, TYPE> b, Value<M, POLY, TYPE> c)
            {
                //    assert(a.i < a.modulus());
                //    assert(b.i < b.modulus());
                //    assert(c.v <= c.N);
                return value(a * b) + c;
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            fma(Index<M, POLY, TYPE> a, Value<M, POLY, TYPE> b, Value<M, POLY, TYPE> c)
            {
                //    assert(a.i < a.modulus());
                //    assert(b.v <= b.N);
                //    assert(c.v <= c.N);
                return !b.v ? c : (value(a * index(b)) + c);
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            fma(Value<M, POLY, TYPE> a, Index<M, POLY, TYPE> b, Value<M, POLY, TYPE> c)
            {
                //    assert(a.v <= a.N);
                //    assert(b.i < b.modulus());
                //    assert(c.v <= c.N);
                return !a.v ? c : (value(index(a) * b) + c);
            }

            template <int M, int POLY, typename TYPE>
            Value<M, POLY, TYPE>
            fma(Value<M, POLY, TYPE> a, Value<M, POLY, TYPE> b, Value<M, POLY, TYPE> c)
            {
                //    assert(a.v <= a.N);
                //    assert(b.v <= b.N);
                //    assert(c.v <= c.N);
                return (!a.v || !b.v) ? c : (value(index(a) * index(b)) + c);
            }
        } // namespace GF

        template <int WIDTH, int POLY, typename TYPE>
        class GaloisField
        {
        public:
            static const int M = WIDTH, Q = 1 << M, N = Q - 1;
            // static_assert(M <= 8 * sizeof(TYPE), "TYPE not wide enough");
            // static_assert(Q == (POLY & ~N), "POLY not of degree Q");
            typedef TYPE value_type;
            typedef GF::Value<M, POLY, TYPE> ValueType;
            typedef GF::Index<M, POLY, TYPE> IndexType;

        private:
            GF::Tables<M, POLY, TYPE> Tables;
        };
    } // namespace CODE
}