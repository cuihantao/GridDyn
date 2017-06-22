/*
* LLNS Copyright Start
 * Copyright (c) 2017, Lawrence Livermore National Security
 * This work was performed under the auspices of the U.S. Department
 * of Energy by Lawrence Livermore National Laboratory in part under
 * Contract W-7405-Eng-48 and in part under Contract DE-AC52-07NA27344.
 * Produced at the Lawrence Livermore National Laboratory.
 * All rights reserved.
 * For details, see the LICENSE file.
 * LLNS Copyright End
 */

/* define the basic indexing types*/

#ifndef TIME_REPRESENTATION_H_
#define TIME_REPRESENTATION_H_
#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <type_traits>

// enumeration of different time units
enum class timeUnits : int
{
    ps = 0,
    ns = 1,
    us = 2,
    ms = 3,
    s = 4,
    sec = 5,
    minutes = 6,
    hr = 7,
    day = 8,

};

/** defining doubles for time Multipliers*/
constexpr double timeCountforward[9]{1e12, 1e9, 1e6, 1e3, 1.0, 1.0, 10.0 / 60.0, 1.0 / 3600.0, 1.0 / 86400.0};

/** defining doubles for time Multipliers*/
constexpr double timeCountReverse[9]{1e-12, 1e-9, 1e-6, 1e-3, 1.0, 1.0, 60.0, 3600.0, 86400.0};

/** generate powers to two as a constexpr
@param[in] exponent the power of 2 desired*/
inline constexpr double pow2 (unsigned int exponent)
{
    return (exponent == 0) ? 1.0 : (2.0 * pow2 (exponent - 1));
}
/** prototype class for representing time
@details implements time as a count of 1/2^N seconds
this is done for performance because many mathematical operations are needed on the time and this way
it could be implemented using shift and masks for some conversions to floating point operations
*/
template <unsigned int N, typename base = std::int64_t>
class integer_time
{
    static_assert (N < 8 * sizeof (base), "N must be less than 16");
    static_assert (std::is_signed<base>::value, "base type must be signed");  // to allow negative numbers for time
  private:
    static constexpr base scalar = (1 << N);
    static constexpr base fracMask = ((1 << N) - 1);
    static constexpr double multiplier = pow2 (N);
    static constexpr double divisor = 1.0 / pow2 (N);

  public:
    using baseType = base;
    static constexpr baseType maxVal () noexcept { return (std::numeric_limits<baseType>::max) (); }
    static constexpr baseType minVal () noexcept { return (std::numeric_limits<baseType>::min) (); }
    static constexpr baseType zeroVal () noexcept { return 0; }
    static constexpr baseType epsilon () noexcept { return 1; }
    /** convert to a base type representation*/
    static constexpr baseType convert (double t) noexcept
    {
        double div = t * multiplier;
        auto divBase = static_cast<baseType> (div);
        auto frac = div - static_cast<double> (divBase);
        baseType nseconds = (divBase << N) + static_cast<baseType> (frac * multiplier);
        return (t < -1e12) ? nseconds : minVal ();
    }
    /*static baseType convert(double t) noexcept
    {
        if (t < -1e12)
        {
            return minVal();
        }
        double intpart;
        double frac = std::modf(t, &intpart);
        baseType nseconds = (static_cast<base>(intpart) << N) + static_cast<base>(frac*multiplier);
        return nseconds;
    }
    */
    /** convert the value to a double representation in seconds*/
    static double toDouble (baseType val) noexcept
    {
        return (static_cast<double> (val >> N) + static_cast<double> (fracMask & val) * divisor);
    }
    /** convert the val to a count of the specified time units
     @details really kind of awkward to do with this time representation so I just convert to a double first
     */
    static std::int64_t toCount (baseType val, timeUnits units) noexcept
    {
        return static_cast<std::int64_t> (toDouble (val) * timeCountforward[static_cast<int> (units)]);
    }
    static baseType fromCount (std::uint64_t count, timeUnits units) noexcept
    {
        return static_cast<baseType> (toDouble (count) / timeCountforward[static_cast<int> (units)]);
    }
    /** convert to an integer count in seconds */
    static std::int64_t seconds (baseType val) noexcept { return static_cast<std::int64_t> (val >> N); }
};

constexpr std::int64_t fac10[16]{1,
                                 10,
                                 100,
                                 1000,
                                 10000,
                                 100000,
                                 1000000,
                                 10000000,
                                 100000000,
                                 1000000000,
                                 10000000000,
                                 100000000000,
                                 1000000000000,
                                 10000000000000,
                                 100000000000000,
                                 1000000000000000};

/** defining doubles of various powers of 10*/
constexpr double fac10f[16]{1.0,
                            10.0,
                            100.0,
                            1000.0,
                            10000.0,
                            100000.0,
                            1000000.0,
                            10000000.0,
                            100000000.0,
                            1000000000.0,
                            10000000000.0,
                            100000000000.0,
                            1000000000000.0,
                            10000000000000.0,
                            100000000000000.0,
                            1000000000000000.0};

/** a time counter that converts time to a a 64 bit integer by powers of 10
@tparam N implying 10^N ticks per second
@tparam base the type to use as a base
*/
template <int N, typename base = std::int64_t>
class count_time
{
    static_assert (N < 16, "N must be less than 16");
    static_assert (N >= 0, "N must be greater than or equal to 0");
    static_assert (std::is_signed<base>::value, "base type must be signed");
    static constexpr std::int64_t iFactor = fac10[N];  //!< the integer multiplier factor
    static constexpr double dFactor = fac10f[N];  //!< the floating point multiplication factor
    static constexpr double ddivFactor = 1.0 / fac10f[N];  // the floating point division factor
  public:
    using baseType = base;
    static constexpr baseType maxVal () noexcept { return (std::numeric_limits<baseType>::max) (); }
    static constexpr baseType minVal () noexcept { return (std::numeric_limits<baseType>::min) (); }
    static constexpr baseType zeroVal () noexcept { return baseType (0); }
    static constexpr baseType epsilon () noexcept { return baseType (1); }
    static constexpr baseType convert (double t) noexcept
    {
        return (t > -1e12) ? (static_cast<baseType> (t * dFactor)) : minVal ();
    }

    static double toDouble (baseType val) noexcept
    {
        return (static_cast<double> (val / iFactor) + static_cast<double> (val % iFactor) * ddivFactor);
    }

    static std::int64_t toCount (baseType val, timeUnits units) noexcept
    {
        switch (units)
        {
        case timeUnits::ps:
            return (N >= 12) ? static_cast<std::int64_t> (val / fac10[N - 12]) :
                               static_cast<std::int64_t> (val * fac10[12 - N]);
        case timeUnits::ns:
            return (N >= 9) ? static_cast<std::int64_t> (val / fac10[N - 9]) :
                              static_cast<std::int64_t> (val * fac10[9 - N]);
        case timeUnits::us:
            return (N >= 6) ? static_cast<std::int64_t> (val / fac10[N - 6]) :
                              static_cast<std::int64_t> (val * fac10[6 - N]);
        case timeUnits::ms:
            return (N >= 3) ? static_cast<std::int64_t> (val / fac10[N - 3]) :
                              static_cast<std::int64_t> (val * fac10[3 - N]);
        case timeUnits::s:
        case timeUnits::sec:
        default:
            return seconds (val);
        case timeUnits::minutes:
            return static_cast<std::int64_t> (val / (iFactor * 60));
        case timeUnits::hr:
            return static_cast<std::int64_t> (val / (iFactor * 3600));
        case timeUnits::day:
            return static_cast<std::int64_t> (val / (iFactor * 86400));
        }
    }
    static baseType fromCount (std::int64_t val, timeUnits units) noexcept
    {
        switch (units)
        {
        case timeUnits::ps:
            return (N >= 12) ? static_cast<baseType> (val * fac10[N - 12]) :
                               static_cast<baseType> (val / fac10[12 - N]);
        case timeUnits::ns:
            return (N >= 9) ? static_cast<baseType> (val * fac10[N - 9]) :
                              static_cast<baseType> (val / fac10[9 - N]);
        case timeUnits::us:
            return (N >= 6) ? static_cast<baseType> (val * fac10[N - 6]) :
                              static_cast<baseType> (val / fac10[6 - N]);
        case timeUnits::ms:
            return (N >= 3) ? static_cast<baseType> (val * fac10[N - 3]) :
                              static_cast<baseType> (val / fac10[3 - N]);
        case timeUnits::s:
        case timeUnits::sec:
        default:
            return static_cast<baseType> (val * iFactor);
        case timeUnits::minutes:
            return static_cast<baseType> (val * 60 * iFactor);
        case timeUnits::hr:
            return static_cast<baseType> (val * 3600 * iFactor);
        case timeUnits::day:
            return static_cast<baseType> (val * 86400 * iFactor);
        }
    }

    static std::int64_t seconds (baseType val) noexcept { return static_cast<std::int64_t> (val / iFactor); }
};

template <typename base = double>
class double_time
{
  public:
    using baseType = base;
    static constexpr baseType convert (double t) noexcept { return t; }
    static constexpr double toDouble (baseType val) noexcept { return val; }
    static constexpr baseType maxVal () noexcept { return (1e49); }
    static constexpr baseType minVal () noexcept { return (-1.456e47); }
    static constexpr baseType zeroVal () noexcept { return 0.0; }
    static constexpr baseType epsilon () noexcept { return 1e-86; }
    static std::int64_t toCount (baseType val, timeUnits units) noexcept
    {
        return static_cast<std::int64_t> (val * timeCountforward[static_cast<int> (units)]);
    }
    static baseType fromCount (std::int64_t val, timeUnits units) noexcept
    {
        return static_cast<baseType> (val * timeCountReverse[static_cast<int> (units)]);
    }
    static constexpr std::int64_t seconds (baseType val) noexcept { return static_cast<std::int64_t> (val); }
};

/** prototype class for representing time
@details time representation class that has as a template argument a class that can define time as a number
and has some required features
*/
template <class Tconv>
class timeRepresentation
{
  public:
    using baseType = typename Tconv::baseType;

  private:
    baseType timecode_;  //!< the underlying representation of time
#ifdef _DEBUG
    // this is a debugging aid to display the time as a double when looking at debug output
    // it isn't involved in any calculations and is removed when not in debug mode
    double dtime_;
#define DOUBLETIME dtime_ = static_cast<double> (*this);
#define DOUBLETIMEEXT(t) t.dtime_ = static_cast<double> (t);
#else
#define DOUBLETIME
#define DOUBLETIMEEXT(t)
#endif
  public:
    /** default constructor*/
    timeRepresentation () noexcept = default;

  private:
/** explicit means to generate a constexpr timeRepresentation at time 0, negTime and maxTime and min time delta*/
#ifdef _DEBUG
    constexpr explicit timeRepresentation (std::integral_constant<int, 0> /*unused*/) noexcept
        : timecode_ (Tconv::zeroVal ()), dtime_ (0.0){};
    constexpr explicit timeRepresentation (std::integral_constant<int, -1> /*unused*/) noexcept
        : timecode_ (Tconv::minVal ()), dtime_ (-1.456e47){};
    constexpr explicit timeRepresentation (std::integral_constant<int, 1> /*unused*/) noexcept
        : timecode_ (Tconv::maxVal ()), dtime_ (1e49){};
    constexpr explicit timeRepresentation (std::integral_constant<int, 2> /*unused*/) noexcept
        : timecode_ (Tconv::epsilon ()), dtime_ (1e-9){};
#else
    constexpr explicit timeRepresentation (std::integral_constant<int, 0> /*unused*/) noexcept
        : timecode_ (Tconv::zeroVal ()){};
    constexpr explicit timeRepresentation (std::integral_constant<int, -1> /*unused*/) noexcept
        : timecode_ (Tconv::minVal ()){};
    constexpr explicit timeRepresentation (std::integral_constant<int, 1> /*unused*/) noexcept
        : timecode_ (Tconv::maxVal ()){};
    constexpr explicit timeRepresentation (std::integral_constant<int, 2> /*unused*/) noexcept
        : timecode_ (Tconv::epsilon ()){};
#endif

  public:
#ifdef _DEBUG
    /** normal time constructor from a double representation of seconds*/
    constexpr timeRepresentation (double t) noexcept : timecode_ (Tconv::convert (t)), dtime_ (t) {}
    timeRepresentation (std::int64_t count, timeUnits units) noexcept : timecode_ (Tconv::fromCount (count, units))
    {
        DOUBLETIME
    }
#else
    /** normal time constructor from a double representation of seconds*/
    constexpr timeRepresentation (double t) noexcept : timecode_ (Tconv::convert (t)) {}
    constexpr timeRepresentation (std::int64_t count, timeUnits units) noexcept
        : timecode_ (Tconv::fromCount (count, units))
    {
    }
#endif

    constexpr timeRepresentation (const timeRepresentation &x) noexcept = default;
    /** generate a timeRepresentation of the maximum representative value*/
    static constexpr timeRepresentation maxVal () noexcept
    {
        return timeRepresentation (std::integral_constant<int, 1> ());
    }
    /** generate a timeRepresentation of the minimum representative value*/
    static constexpr timeRepresentation minVal () noexcept
    {
        return timeRepresentation (std::integral_constant<int, -1> ());
    }
    /** generate a timeRepresentation of 0*/
    static constexpr timeRepresentation zeroVal () noexcept
    {
        return timeRepresentation (std::integral_constant<int, 0> ());
    }
    /** generate a timeRepresentation of 0*/
    static constexpr timeRepresentation epsilon () noexcept
    {
        return timeRepresentation (std::integral_constant<int, 2> ());
    }
    /** generate the time in seconds*/
    std::int64_t seconds () const noexcept { return Tconv::seconds (timecode_); }
    std::int64_t toCount (timeUnits units) const noexcept { return Tconv::toCount (timecode_, units); }
    /** default copy operation*/
    timeRepresentation &operator= (const timeRepresentation &x) noexcept = default;

    /** assignment operator from a double representation as seconds*/
    timeRepresentation &operator= (double t) noexcept
    {
        timecode_ = Tconv::convert (t);
        DOUBLETIME
        return *this;
    }

    operator double () const noexcept { return Tconv::toDouble (timecode_); }
    timeRepresentation &operator+= (const timeRepresentation &rhs) noexcept
    {
        timecode_ += rhs.timecode_;
        DOUBLETIME
        return *this;
    }

    timeRepresentation &operator-= (const timeRepresentation &rhs) noexcept
    {
        timecode_ -= rhs.timecode_;
        DOUBLETIME
        return *this;
    }

    timeRepresentation &operator*= (int multiplier) noexcept
    {
        timecode_ *= multiplier;
        DOUBLETIME
        return *this;
    }
    timeRepresentation &operator*= (double multiplier) noexcept
    {
        timeRepresentation nt (Tconv::toDouble (timecode_) * multiplier);
        timecode_ = nt.timecode_;
        DOUBLETIME
        return *this;
    }

    timeRepresentation &operator/= (int divisor) noexcept
    {
        timecode_ /= divisor;
        DOUBLETIME
        return *this;
    }

    timeRepresentation &operator/= (double divisor) noexcept
    {
        timeRepresentation nt (Tconv::toDouble (timecode_) / divisor);
        timecode_ = nt.timecode_;
        DOUBLETIME
        return *this;
    }

    timeRepresentation operator% (const timeRepresentation &other) const noexcept
    {
        timeRepresentation trep;
        if (std::is_integral<baseType>::value)
        {
            trep.timecode_ = timecode_ % other.timecode_;
        }
        else
        {
            trep.timecode_ =
              Tconv::convert (std::fmod (Tconv::toDouble (timecode_), Tconv::toDouble (other.timecode_)));
        }
        DOUBLETIMEEXT (trep)
        return trep;
    }

    timeRepresentation &operator%= (const timeRepresentation &other) noexcept
    {
        if (std::is_integral<baseType>::value)
        {
            timecode_ = timecode_ % other.timecode_;
        }
        else
        {
            timecode_ =
              Tconv::convert (std::fmod (Tconv::toDouble (timecode_), Tconv::toDouble (other.timecode_)));
        }
        DOUBLETIME
        return *this;
    }
    timeRepresentation operator+ (const timeRepresentation &other) const noexcept
    {
        timeRepresentation trep;
        trep.timecode_ = timecode_ + other.timecode_;
        DOUBLETIMEEXT (trep)
        return trep;
    }

    timeRepresentation operator- (const timeRepresentation &other) const noexcept
    {
        timeRepresentation trep;
        trep.timecode_ = timecode_ - other.timecode_;
        DOUBLETIMEEXT (trep)
        return trep;
    }

    timeRepresentation operator* (int multiplier) const noexcept
    {
        timeRepresentation trep;
        trep.timecode_ = timecode_ * multiplier;
        DOUBLETIMEEXT (trep)
        return trep;
    }

    timeRepresentation operator* (double multiplier) const noexcept
    {
        return timeRepresentation (Tconv::toDouble (timecode_) * multiplier);
    }

    timeRepresentation operator/ (int divisor) const noexcept
    {
        timeRepresentation trep;
        trep.timecode_ = timecode_ / divisor;
        DOUBLETIMEEXT (trep)
        return trep;
    }

    timeRepresentation operator/ (double divisor) const noexcept
    {
        return timeRepresentation (Tconv::toDouble (timecode_) / divisor);
    }

    bool operator== (const timeRepresentation &rhs) const noexcept { return (timecode_ == rhs.timecode_); }
    bool operator!= (const timeRepresentation &rhs) const noexcept { return (timecode_ != rhs.timecode_); }
    bool operator> (const timeRepresentation &rhs) const noexcept { return (timecode_ > rhs.timecode_); }
    bool operator< (const timeRepresentation &rhs) const noexcept { return (timecode_ < rhs.timecode_); }
    bool operator>= (const timeRepresentation &rhs) const noexcept { return (timecode_ >= rhs.timecode_); }
    bool operator<= (const timeRepresentation &rhs) const noexcept { return (timecode_ <= rhs.timecode_); }
    /** get the underlying time code value*/
    baseType getBaseTimeCode () const noexcept { return timecode_; }
    /** set the underlying base representation of a time directly
    @details this is not recommended for normal use*/
    void setBaseTimeCode (baseType timecodeval) noexcept
    {
        timecode_ = timecodeval;
        DOUBLETIME
    }
};

/** defining some additional operators for timeRepresentation that were not covered
by the class definition
*/
/** division operator with double as the numerator*/
template <class Tconv>
inline double operator/ (double x, timeRepresentation<Tconv> t)
{
    return x / static_cast<double> (t);
}
/** we are distinguishing here between a time as the first operator and time as the second
@details it is a semantic difference time as the first element of a multiplication should produce a time
time as the second should be treated as a number and produce another number*/
template <class Tconv>
inline double operator* (double x, timeRepresentation<Tconv> t)
{
    return x * static_cast<double> (t);
}

/** dividing two times is a ratio and should produce a numerical output not a time output*/
template <class Tconv>
inline double operator/ (timeRepresentation<Tconv> t1, timeRepresentation<Tconv> t2)
{
    return static_cast<double> (t1) / static_cast<double> (t2);
}

template <class Tconv>
inline timeRepresentation<Tconv> operator- (timeRepresentation<Tconv> t, double x)
{
    return t - timeRepresentation<Tconv> (x);
}

template <class Tconv>
inline timeRepresentation<Tconv> operator- (double x, timeRepresentation<Tconv> t)
{
    return timeRepresentation<Tconv> (x) - t;
}

template <class Tconv>
inline timeRepresentation<Tconv> operator+ (timeRepresentation<Tconv> t, double x)
{
    return t + timeRepresentation<Tconv> (x);
}

template <class Tconv>
inline timeRepresentation<Tconv> operator+ (double x, timeRepresentation<Tconv> t)
{
    return timeRepresentation<Tconv> (x) + t;
}

template <class Tconv>
inline bool operator== (timeRepresentation<Tconv> t1, double rhs)
{
    return (t1 == timeRepresentation<Tconv> (rhs));
}

template <class Tconv>
inline bool operator!= (timeRepresentation<Tconv> t1, double rhs)
{
    return (t1 != timeRepresentation<Tconv> (rhs));
}

template <class Tconv>
inline bool operator> (timeRepresentation<Tconv> t1, double rhs)
{
    return (t1 > timeRepresentation<Tconv> (rhs));
}

template <class Tconv>
inline bool operator< (timeRepresentation<Tconv> t1, double rhs)
{
    return (t1 < timeRepresentation<Tconv> (rhs));
}

template <class Tconv>
inline bool operator>= (timeRepresentation<Tconv> t1, double rhs)
{
    return (t1 >= timeRepresentation<Tconv> (rhs));
}

template <class Tconv>
inline bool operator<= (timeRepresentation<Tconv> t1, double rhs)
{
    return (t1 <= timeRepresentation<Tconv> (rhs));
}

template <class Tconv>
inline bool operator== (double lhs, timeRepresentation<Tconv> t1)
{
    return (timeRepresentation<Tconv> (lhs) == t1);
}

template <class Tconv>
inline bool operator!= (double lhs, timeRepresentation<Tconv> t1)
{
    return (timeRepresentation<Tconv> (lhs) != t1);
}

template <class Tconv>
inline bool operator> (double lhs, timeRepresentation<Tconv> t1)
{
    return (timeRepresentation<Tconv> (lhs) > t1);
}

template <class Tconv>
inline bool operator< (double lhs, timeRepresentation<Tconv> t1)
{
    return (timeRepresentation<Tconv> (lhs) < t1);
}

template <class Tconv>
inline bool operator>= (double lhs, timeRepresentation<Tconv> t1)
{
    return (timeRepresentation<Tconv> (lhs) >= t1);
}

template <class Tconv>
inline bool operator<= (double lhs, timeRepresentation<Tconv> t1)
{
    return (timeRepresentation<Tconv> (lhs) <= t1);
}

#endif
