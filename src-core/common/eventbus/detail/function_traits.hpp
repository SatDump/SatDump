#pragma once

#include <functional>

/**
 * `member_function_traits originally from here (PMF_traits struct): https://github.com/KonanM/CppProperties/blob/master/include/cppproperties/Signal.h
 * `function_traits` taken from answer here: https://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda
 * Original code is here: https://github.com/kennytm/utils/blob/master/traits.hpp
 */ 
namespace dp
{
    namespace detail
    {
        template<typename>
        struct member_function_traits
        {
            using member_type = void;
            using class_type = void;
        };

        template<typename T, typename U>
        struct member_function_traits<U T::*>
        {
            using member_type = U;
            using class_type = T;
        };

        template <typename C, typename R, typename... A>
        struct memfn_type
        {
            typedef typename std::conditional<
                std::is_const<C>::value,
                typename std::conditional<
                std::is_volatile<C>::value,
                R(C::*)(A...) const volatile,
                R(C::*)(A...) const
                >::type,
                typename std::conditional<
                std::is_volatile<C>::value,
                R(C::*)(A...) volatile,
                R(C::*)(A...)
                >::type
            >::type type;
        };

        template<typename T>
        struct function_traits : public function_traits<decltype(&T::operator())> {};

        template <typename ReturnType, typename... Args>
        struct function_traits<ReturnType(Args...)>
        {
            /**
            .. type:: type result_type
                The type returned by calling an instance of the function object type *F*.
            */
            typedef ReturnType result_type;

            /**
            .. type:: type function_type
                The function type (``R(T...)``).
            */
            typedef ReturnType function_type(Args...);

            /**
            .. type:: type member_function_type<OwnerType>
                The member function type for an *OwnerType* (``R(OwnerType::*)(T...)``).
            */
            template <typename OwnerType>
            using member_function_type = typename memfn_type<
                typename std::remove_pointer<typename std::remove_reference<OwnerType>::type>::type,
                ReturnType, Args...
            >::type;

            /**
            .. data:: static const size_t arity
                Number of arguments the function object will take.
            */
            enum { arity = sizeof...(Args) };

            /**
            .. type:: type arg<n>::type
                The type of the *n*-th argument.
            */
            template <size_t i>
            struct arg
            {
                typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
            };
        };

        template <typename ReturnType, typename... Args>
        struct function_traits<ReturnType(*)(Args...)>
            : public function_traits<ReturnType(Args...)>
        {};

        template <typename ClassType, typename ReturnType, typename... Args>
        struct function_traits<ReturnType(ClassType::*)(Args...)>
            : public function_traits<ReturnType(Args...)>
        {
            typedef ClassType& owner_type;
        };

        template <typename ClassType, typename ReturnType, typename... Args>
        struct function_traits<ReturnType(ClassType::*)(Args...) const>
            : public function_traits<ReturnType(Args...)>
        {
            typedef const ClassType& owner_type;
        };

        template <typename ClassType, typename ReturnType, typename... Args>
        struct function_traits<ReturnType(ClassType::*)(Args...) volatile>
            : public function_traits<ReturnType(Args...)>
        {
            typedef volatile ClassType& owner_type;
        };

        template <typename ClassType, typename ReturnType, typename... Args>
        struct function_traits<ReturnType(ClassType::*)(Args...) const volatile>
            : public function_traits<ReturnType(Args...)>
        {
            typedef const volatile ClassType& owner_type;
        };

        template <typename FunctionType>
        struct function_traits<std::function<FunctionType>>
            : public function_traits<FunctionType>
        {};

        template <typename T>
        struct function_traits<T&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<const T&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<volatile T&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<const volatile T&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<T&&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<const T&&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<volatile T&&> : public function_traits<T> {};
        template <typename T>
        struct function_traits<const volatile T&&> : public function_traits<T> {};
    }
}
