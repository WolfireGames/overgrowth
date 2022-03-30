/**
 * @brief  Additional ensures for scientific/engineering applications.
 * @author Joerg <yogi2005@users.sourceforge.net>
 * @date   07/04/2008
 */
#ifndef TUT_Float_H_GUARD
#define TUT_Float_H_GUARD

#include <limits>
#include <iostream>

namespace tut
{
    namespace detail
    {
        template<bool Predicate, typename Then, typename Else>
        struct If
        {
            typedef Else type;
        };

        template<typename Then, typename Else>
        struct If<true,Then,Else>
        {
            typedef Then type;
        };

        template<typename T>
        struct fpt_traits
        {
            struct StdNumericLimitsNotAvailable {};
            static const StdNumericLimitsNotAvailable static_check[ std::numeric_limits<T>::is_specialized ];

            static const T zero;

            typedef typename If<std::numeric_limits<T>::is_integer,
                                double,
                                T>::type Result;

            static T abs(const T &arg)
            {
                if(arg < zero)
                    return zero - arg;
                else
                    return arg;
            }

            static T sig(const T &arg)
            {
                if(arg < zero)
                    return -1;
                else
                    return 1;
            }

            static inline Result div(const Result &number, const T &divisor)
            {
                static_cast<void>(static_check);

                if(number == zero && divisor == zero)
                    return std::numeric_limits<Result>::quiet_NaN();

                if(number == zero)
                    return zero;

                if(divisor == zero)
                    return sig(number) * std::numeric_limits<Result>::infinity();

                assert(zero < number);
                assert(zero < divisor);

                // Avoid underflow
                if(static_cast<T>(1) < abs(divisor))
                {
                    // number / divisor < min <=> number < min * divisor
                    if( abs(number) < abs(divisor) * std::numeric_limits<T>::min())
                    {
                        return sig(divisor) * sig(number) * std::numeric_limits<T>::min();
                    }
                }

                // Avoid overflow
                if( abs(divisor) < static_cast<T>(1))
                {
                    // number / divisor > max <=> number > max * divisor
                    if( abs(divisor) * std::numeric_limits<T>::max() < abs(number))
                    {
                        return sig(divisor) * sig(number) * std::numeric_limits<T>::max();
                    }
                }

                return number / divisor;
            }
        };

        template<typename T>
        const typename fpt_traits<T>::StdNumericLimitsNotAvailable
            fpt_traits<T>::static_check[ std::numeric_limits<T>::is_specialized ] = { {} };

        template<typename T>
        const T fpt_traits<T>::zero = static_cast<T>(0);

        template<typename T, typename U>
        bool check_tolerance(T actual, T expected, U fraction)
        {
            typename fpt_traits<T>::Result diff = fpt_traits<T>::div( fpt_traits<T>::abs( expected - actual ),
                                                                      fpt_traits<T>::abs( expected ) );

            return (diff == fraction) || (diff < fraction);
        }

    } // namespace detail

    template<typename T, typename U>
    void ensure_close(const char* msg, const T& actual, const T& expected, const U& tolerance )
    {
        typedef detail::fpt_traits<U> Traits;

        typename Traits::Result fraction = Traits::div( Traits::abs(static_cast<typename Traits::Result>(tolerance)),
                                                        static_cast<typename Traits::Result>(100) );
        if( !detail::check_tolerance(actual, expected, fraction) )
        {
            std::ostringstream ss;
            ss << ( msg ? msg : "" )
            << ( msg ? ": " : "" )
            << "expected `"
            << expected
            << "` and actual `"
            << actual
            << "` differ more than "
            << tolerance
            << "%";
             throw failure( ss.str().c_str() );
        }
    }

    template<typename T, typename Tolerance>
    void ensure_close(const T& actual, const T& expected, const Tolerance& tolerance )
    {
        ensure_close( 0, actual, expected, tolerance );
    }

    template<typename T, typename U>
    void ensure_close_fraction(const char* msg, const T& actual, const T& expected, const U& fraction)
    {
        typedef char StdNumericLimitsNotAvailable;
        const StdNumericLimitsNotAvailable static_check[ std::numeric_limits<U>::is_specialized ] = { 0 };
        static_cast<void>(static_check);

        typedef typename detail::If<std::numeric_limits<U>::is_integer,
                                    double,
                                    U>::type Tolerance;

        if( !detail::check_tolerance(actual, expected, fraction) )
        {
            std::ostringstream ss;
            ss << ( msg ? msg : "" )
            << ( msg ? ": " : "" )
            << "expected `"
            << expected
            << "` and actual `"
            << actual
            << "` differ more than fraction `"
            << fraction
            << "`";
            throw failure( ss.str().c_str() );
        }
    }

    template<typename T>
    void ensure_close_fraction( const char* msg, const T& actual, const T& expected, const int& tolerance )
    {
        ensure_close(msg, actual, expected, double(tolerance));
    }

    template< typename T, typename Tolerance>
    void ensure_close_fraction(const T& actual, const T& expected, const Tolerance& fraction)
    {
        ensure_close_fraction( 0, actual, expected, fraction );
    }

} // namespace tut

#endif

