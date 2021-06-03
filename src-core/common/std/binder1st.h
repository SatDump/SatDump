#pragma once

// This is not present on Android, so we have to provide it ourselves
namespace std
{
    template <typename _Operation>
    class binder1st
        : public unary_function<typename _Operation::second_argument_type,
                                typename _Operation::result_type>
    {
    protected:
        _Operation op;
        typename _Operation::first_argument_type value;

    public:
        binder1st(const _Operation &__x,
                  const typename _Operation::first_argument_type &__y)
            : op(__x), value(__y) {}

        typename _Operation::result_type
        operator()(const typename _Operation::second_argument_type &__x) const
        {
            return op(value, __x);
        }

        // _GLIBCXX_RESOLVE_LIB_DEFECTS
        // 109.  Missing binders for non-const sequence elements
        typename _Operation::result_type
        operator()(typename _Operation::second_argument_type &__x) const
        {
            return op(value, __x);
        }
    };

    /// One of the @link binders binder functors@endlink.
    template <typename _Operation, typename _Tp>
    inline binder1st<_Operation>
    bind1st(const _Operation &__fn, const _Tp &__x)
    {
        typedef typename _Operation::first_argument_type _Arg1_type;
        return binder1st<_Operation>(__fn, _Arg1_type(__x));
    }
}