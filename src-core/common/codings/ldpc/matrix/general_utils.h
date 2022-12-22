/*!
 * \file
 * \brief Functions for general operations.
 */
#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <typeindex>
#include <memory>

namespace codings
{
	namespace ldpc
	{
        std::vector<std::string> split(const std::string &s, char delim);

        std::vector<std::string> split(const std::string &s);

        void getline(std::istream &file, std::string &line);

        template <typename R = float>
        R sigma_to_esn0(const R sigma, const int upsample_factor = 1);

        template <typename R = float>
        R esn0_to_sigma(const R esn0, const int upsample_factor = 1);

        template <typename R = float>
        R esn0_to_ebn0(const R esn0, const R bit_rate = 1, const int bps = 1);

        template <typename R = float>
        R ebn0_to_esn0(const R ebn0, const R bit_rate = 1, const int bps = 1);

        /* Transform a Matlab style description of a range into a vector.
         * For ex: "0:1.2,3.4:0.2:4.4" gives the vector "0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1 1.1 1.2 3.4 3.6 3.8 4 4.2 4.4"
         * The first dimension of 'range_description' can have any size.
         * The second is between 1 to 3. If 1, then a value alone, else the first element is the min value, the last is the max value. The one between if any
         * is the step to use else use the step 'default_step'.
         */
        template <typename R = float>
        std::vector<R> generate_range(const std::vector<std::vector<R>> &range_description, const R default_step = (R)0.1);

        /*
         * Get the nearest position of value in range [first, last[.
         * the reference data must be sorted and strictly monotonic increasing
         * If value goes out of x_data range, then return the left or right limit value in function of the violated one.
         */
        template <typename BidirectionalIterator, typename T>
        inline BidirectionalIterator get_closest(BidirectionalIterator first, BidirectionalIterator last, const T &value);

        template <typename BidirectionalIterator, typename T>
        inline std::size_t get_closest_index(BidirectionalIterator first, BidirectionalIterator last, const T &value);

        /*
         * Sort 'vec_abscissa' in ascending order and move the matching ordinate of 'vec_ordinate' by the same time.
         */
        template <typename Ta, typename To>
        inline void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<To> &vec_ordinate);

        template <typename Ta, typename To>
        inline void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<std::vector<To>> &vec_ordinate);

        /*
         * Sort 'vec_abscissa' in function of the comp function and move the matching ordinate of 'vec_ordinate' by the same time.
         */
        template <typename Ta, typename To, class Compare>
        inline void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<To> &vec_ordinate, Compare comp);

        template <typename Ta, typename To, class Compare>
        inline void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<std::vector<To>> &vec_ordinate, Compare comp);

        /*
         * Eliminates all but the first element from every consecutive group of equivalent elements from the 'vec_abscissa' vector
         * and remove the matching ordinate of 'vec_ordinate' by the same time.
         * 'vec_abscissa' and 'vec_ordinate' are resized to their new length.
         */
        template <typename Ta, typename To>
        inline void mutual_unique(std::vector<Ta> &vec_abscissa, std::vector<To> &vec_ordinate);

        template <typename Ta, typename To>
        inline void mutual_unique(std::vector<Ta> &vec_abscissa, std::vector<std::vector<To>> &vec_ordinate);

        /*
         * Convert the number of seconds into a __h__'__ string format
         * For exemple 3662 secondes would be displayed as "01h01'02".
         * "secondes" is first converted into a "int" type to be processed
         */
        template <typename T>
        std::string get_time_format(T secondes);

        /*
         * calculate offset of a class member at compile time
         * Source : https://stackoverflow.com/a/20141143/7219905
         */
        template <typename T, typename U>
        constexpr size_t offsetOf(U T::*member);

        template <typename T>
        std::vector<T *> convert_to_ptr(const std::vector<std::unique_ptr<T>> &v);

        template <typename T>
        std::vector<T *> convert_to_ptr(const std::vector<std::shared_ptr<T>> &v);

        template <typename T>
        void check_LUT(const std::vector<T> &LUT, const std::string &LUT_name = "LUT", const size_t LUT_size = 0);

        size_t compute_bytes(const size_t n_elmts, const std::type_index type);

        std::vector<size_t> compute_bytes(const std::vector<size_t> &n_elmts, const std::vector<std::type_index> &type);
    }
}

namespace codings
{
	namespace ldpc
	{
        template <typename BidirectionalIterator, typename T>
        BidirectionalIterator get_closest(BidirectionalIterator first, BidirectionalIterator last, const T &value)
        { // https://stackoverflow.com/a/701141/7219905

            auto before = std::lower_bound(first, last, value);

            if (before == first)
                return first;
            if (before == last)
                return --last; // iterator must be bidirectional

            auto after = before;
            --before;

            return (*after - value) < (value - *before) ? after : before;
        }

        template <typename BidirectionalIterator, typename T>
        std::size_t get_closest_index(BidirectionalIterator first, BidirectionalIterator last, const T &value)
        {
            return std::distance(first, get_closest(first, last, value));
        }

        template <typename Ta, typename To>
        void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<To> &vec_ordinate)
        {
            mutual_sort(vec_abscissa, vec_ordinate, [](const Ta &a, const Ta &b)
                        { return a < b; });
        }

        template <typename Ta, typename To>
        void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<std::vector<To>> &vec_ordinate)
        {
            mutual_sort(vec_abscissa, vec_ordinate, [](const Ta &a, const Ta &b)
                        { return a < b; });
        }

        template <typename Ta, typename To, class Compare>
        void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<To> &vec_ordinate, Compare comp)
        {
            assert(vec_abscissa.size() == vec_ordinate.size());

            for (unsigned i = 1; i < vec_abscissa.size(); i++)
                for (unsigned j = i; j > 0 && comp(vec_abscissa[j], vec_abscissa[j - 1]); j--)
                {
                    std::swap(vec_abscissa[j], vec_abscissa[j - 1]); // order the x position
                    std::swap(vec_ordinate[j], vec_ordinate[j - 1]); // the y follow their x, moving the same way
                }
        }

        template <typename Ta, typename To, class Compare>
        void mutual_sort(std::vector<Ta> &vec_abscissa, std::vector<std::vector<To>> &vec_ordinate, Compare comp)
        {
            for (unsigned i = 1; i < vec_abscissa.size(); i++)
                for (unsigned j = i; j > 0 && comp(vec_abscissa[j], vec_abscissa[j - 1]); j--)
                {
                    std::swap(vec_abscissa[j], vec_abscissa[j - 1]); // order the x position

                    for (unsigned k = 0; k < vec_ordinate.size(); k++)
                        std::swap(vec_ordinate[k][j], vec_ordinate[k][j - 1]); // the y follow their x, moving the same way
                }
        }

        template <typename Ta, typename To>
        void mutual_unique(std::vector<Ta> &vec_abscissa, std::vector<To> &vec_ordinate)
        {
            assert(vec_abscissa.size() == vec_ordinate.size());

            unsigned r = 0;

            for (unsigned i = 1; i < vec_abscissa.size(); i++)
            {
                if ((vec_abscissa[r] != vec_abscissa[i]) && (++r != i))
                {
                    vec_abscissa[r] = std::move(vec_abscissa[i]);
                    vec_ordinate[r] = std::move(vec_ordinate[i]);
                }
            }

            if (vec_abscissa.size() != 0 && r == 0)
                r = 1;

            vec_abscissa.resize(r);
            vec_ordinate.resize(r);
        }

        template <typename Ta, typename To>
        void mutual_unique(std::vector<Ta> &vec_abscissa, std::vector<std::vector<To>> &vec_ordinate)
        {
            unsigned r = 0;

            for (unsigned i = 1; i < vec_abscissa.size(); i++)
            {
                if ((vec_abscissa[r] != vec_abscissa[i]) && (++r != i))
                {
                    vec_abscissa[r] = std::move(vec_abscissa[i]);
                    for (unsigned k = 0; k < vec_ordinate.size(); k++)
                        vec_ordinate[k][r] = std::move(vec_ordinate[k][i]);
                }
            }

            if (vec_abscissa.size() != 0 && r == 0)
                r = 1;

            vec_abscissa.resize(r);
            vec_ordinate.resize(r);
        }

        template <typename T>
        std::string get_time_format(T secondes)
        {
            auto ss = (int)secondes % 60;
            auto mm = ((int)(secondes / 60.f) % 60);
            auto hh = (int)(secondes / 3600.f);

            std::stringstream time_format;

            time_format << std::setw(2) << std::setfill('0') << hh << "h";
            time_format << std::setw(2) << std::setfill('0') << mm << "'";
            time_format << std::setw(2) << std::setfill('0') << ss;

            return time_format.str();
        }

        template <typename T, typename U>
        constexpr size_t offsetOf(U T::*member)
        {
            return (char *)&((T *)nullptr->*member) - (char *)nullptr;
        }

        template <typename T>
        std::vector<T *> convert_to_ptr(const std::vector<std::unique_ptr<T>> &v)
        {
            std::vector<T *> v2;
            for (auto &e : v)
                v2.push_back(e.get());
            return v2;
        }

        template <typename T>
        std::vector<T *> convert_to_ptr(const std::vector<std::shared_ptr<T>> &v)
        {
            std::vector<T *> v2;
            for (auto &e : v)
                v2.push_back(e.get());
            return v2;
        }

    }
}
