// The MIT License (MIT)
//
// Copyright (c) 2018 Mateusz Pusz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include "unit.h"
#include <limits>

namespace units {

  // is_quantity

  template<Dimension D, Unit U, typename Rep>
  requires Same<D, typename U::dimension> class quantity;

  namespace detail {

    template<typename T>
    struct is_quantity : std::false_type {
    };

    template<Dimension D, Unit U, typename Rep>
    struct is_quantity<quantity<D, U, Rep>> : std::true_type {
    };

  }  // namespace detail

  template<typename T>
  concept bool Quantity = detail::is_quantity<T>::value;

  // treat_as_floating_point

  template<class Rep>
  struct treat_as_floating_point : std::is_floating_point<Rep> {
  };

  template<class Rep>
  inline constexpr bool treat_as_floating_point_v = treat_as_floating_point<Rep>::value;

  // quantity_cast

  namespace detail {

    template<Quantity To, Ratio CR, typename CRep, bool NumIsOne = false, bool DenIsOne = false>
    struct quantity_cast_impl {
      template<Dimension D, Ratio R, typename Rep>
      static constexpr To cast(const quantity<D, unit<D, R>, Rep>& q)
      {
        return To(static_cast<typename To::rep>(static_cast<CRep>(q.count()) * static_cast<CRep>(CR::num) /
                                                static_cast<CRep>(CR::den)));
      }
    };

    template<Quantity To, Ratio CR, typename CRep>
    struct quantity_cast_impl<To, CR, CRep, true, true> {
      template<Dimension D, Ratio R, typename Rep>
      static constexpr To cast(const quantity<D, unit<D, R>, Rep>& q)
      {
        return To(static_cast<typename To::rep>(q.count()));
      }
    };

    template<Quantity To, Ratio CR, typename CRep>
    struct quantity_cast_impl<To, CR, CRep, true, false> {
      template<Dimension D, Ratio R, typename Rep>
      static constexpr To cast(const quantity<D, unit<D, R>, Rep>& q)
      {
        return To(static_cast<typename To::rep>(static_cast<CRep>(q.count()) / static_cast<CRep>(CR::den)));
      }
    };

    template<Quantity To, Ratio CR, typename CRep>
    struct quantity_cast_impl<To, CR, CRep, false, true> {
      template<Dimension D, Ratio R, typename Rep>
      static constexpr To cast(const quantity<D, unit<D, R>, Rep>& q)
      {
        return To(static_cast<typename To::rep>(static_cast<CRep>(q.count()) * static_cast<CRep>(CR::num)));
      }
    };

  }  // namespace detail

  template<Quantity To, Dimension D, Unit U, typename Rep>
  requires Same<typename To::dimension, D> constexpr To quantity_cast(const quantity<D, U, Rep>& q)
  {
    using c_ratio = std::ratio_divide<typename U::ratio, typename To::unit::ratio>;
    using c_rep = std::common_type_t<typename To::rep, Rep, intmax_t>;
    using cast = detail::quantity_cast_impl<To, c_ratio, c_rep, c_ratio::num == 1, c_ratio::den == 1>;
    return cast::cast(q);
  }

  // quantity_values

  template<typename Rep>
  struct quantity_values {
    static constexpr Rep zero() { return Rep(0); }
    static constexpr Rep max() { return std::numeric_limits<Rep>::max(); }
    static constexpr Rep min() { return std::numeric_limits<Rep>::lowest(); }
  };

  // quantity

  template<Dimension D, Unit U, typename Rep>
  requires Same<D, typename U::dimension> class quantity {
    Rep value_;

  public:
    using dimension = D;
    using unit = U;
    using rep = Rep;

    static_assert(!detail::is_quantity<Rep>::value, "rep cannot be a quantity");

    quantity() = default;
    quantity(const quantity&) = default;

    template<class Rep2>
        requires ConvertibleTo<Rep2, rep> &&
        (treat_as_floating_point_v<rep> || !treat_as_floating_point_v<Rep2>)constexpr explicit quantity(const Rep2& r)
        : value_{static_cast<rep>(r)}
    {
    }

    template<Quantity Q2>
        requires Same<dimension, typename Q2::dimension>&& ConvertibleTo<typename Q2::rep, rep> &&
        (treat_as_floating_point_v<rep> ||
         (std::ratio_divide<typename Q2::unit::ratio, typename unit::ratio>::den == 1 &&
          !treat_as_floating_point_v<typename Q2::rep>)) constexpr quantity(const Q2& q)
        : value_{quantity_cast<quantity>(q).count()}
    {
    }

    quantity& operator=(const quantity& other) = default;

    constexpr rep count() const noexcept { return value_; }

    static constexpr quantity zero() { return quantity(quantity_values<Rep>::zero()); }
    static constexpr quantity min() { return quantity(quantity_values<Rep>::min()); }
    static constexpr quantity max() { return quantity(quantity_values<Rep>::max()); }

    constexpr std::common_type_t<quantity> operator+() const { return quantity(*this); }
    constexpr std::common_type_t<quantity> operator-() const { return quantity(-count()); }

    constexpr quantity& operator++()
    {
      ++value_;
      return *this;
    }
    constexpr quantity operator++(int) { return quantity(value_++); }

    constexpr quantity& operator--()
    {
      --value_;
      return *this;
    }
    constexpr quantity operator--(int) { return quantity(value_--); }

    constexpr quantity& operator+=(const quantity& q)
    {
      value_ += q.count();
      return *this;
    }

    constexpr quantity& operator-=(const quantity& q)
    {
      value_ -= q.count();
      return *this;
    }

    constexpr quantity& operator*=(const rep& rhs)
    {
      value_ *= rhs;
      return *this;
    }

    constexpr quantity& operator/=(const rep& rhs)
    {
      value_ /= rhs;
      return *this;
    }

    constexpr quantity& operator%=(const rep& rhs)
    {
      value_ %= rhs;
      return *this;
    }

    constexpr quantity& operator%=(const quantity& q)
    {
      value_ %= q.count();
      return *this;
    }
  };

  // clang-format off
  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>
  constexpr operator+(const quantity<D, U1, Rep1>& lhs,
                      const quantity<D, U2, Rep2>& rhs)
  {
    using ret = std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>;
    return ret(ret(lhs).count() + ret(rhs).count());
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>
  constexpr operator-(const quantity<D, U1, Rep1>& lhs,
                      const quantity<D, U2, Rep2>& rhs)
  {
    using ret = std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>;
    return ret(ret(lhs).count() - ret(rhs).count());
  }

  template<Dimension D, Unit U, typename Rep1, typename Rep2>
  quantity<D, U, std::common_type_t<Rep1, Rep2>>
  constexpr operator*(const quantity<D, U, Rep1>& q,
                      const Rep2& v)
  {
    using ret = quantity<D, U, std::common_type_t<Rep1, Rep2>>;
    return ret(ret(q).count() * v);
  }

  template<typename Rep1, Dimension D, Unit U, typename Rep2>
  quantity<D, U, std::common_type_t<Rep1, Rep2>>
  constexpr operator*(const Rep1& v,
                      const quantity<D, U, Rep2>& q)
  {
    return q * v;
  }

  template<Dimension D1, Unit U1, typename Rep1, Dimension D2, Unit U2, typename Rep2>
      requires treat_as_floating_point_v<std::common_type_t<Rep1, Rep2>> || std::ratio_multiply<typename U1::ratio, typename U2::ratio>::den == 1
  quantity<dimension_multiply_t<D1, D2>, unit<dimension_multiply_t<D1, D2>, std::ratio_multiply<typename U1::ratio, typename U2::ratio>>, std::common_type_t<Rep1, Rep2>>
  constexpr operator*(const quantity<D1, U1, Rep1>& lhs,
                      const quantity<D2, U2, Rep2>& rhs)
  {
    using dim = dimension_multiply_t<D1, D2>;
    using ret = quantity<dim, unit<dim, std::ratio_multiply<typename U1::ratio, typename U2::ratio>>, std::common_type_t<Rep1, Rep2>>;
    return ret(lhs.count() * rhs.count());
  }

  template<typename Rep1, Exponent... E, Unit U, typename Rep2>
  quantity<dimension<exp_invert_t<E>...>, unit<dimension<exp_invert_t<E>...>, std::ratio<U::ratio::den, U::ratio::num>>, std::common_type_t<Rep1, Rep2>>
  constexpr operator/(const Rep1& v,
                      const quantity<dimension<E...>, U, Rep2>& q)
  {
    using dim = dimension<exp_invert_t<E>...>;
    using ret = quantity<dim, unit<dim, std::ratio<U::ratio::den, U::ratio::num>>, std::common_type_t<Rep1, Rep2>>;
    using den = quantity<dimension<E...>, U, std::common_type_t<Rep1, Rep2>>;
    return ret(v / den(q).count());
  }

  template<Dimension D, Unit U, typename Rep1, typename Rep2>
  quantity<D, U, std::common_type_t<Rep1, Rep2>>
  constexpr operator/(const quantity<D, U, Rep1>& q,
                      const Rep2& v)
  {
    using ret = quantity<D, U, std::common_type_t<Rep1, Rep2>>;
    return ret(ret(q).count() / v);
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  std::common_type_t<Rep1, Rep2>
  constexpr operator/(const quantity<D, U1, Rep1>& lhs,
                      const quantity<D, U2, Rep2>& rhs)
  {
    using cq = std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>;
    return cq(lhs).count() / cq(rhs).count();
  }

  template<Dimension D1, Unit U1, typename Rep1, Dimension D2, Unit U2, typename Rep2>
      requires treat_as_floating_point_v<std::common_type_t<Rep1, Rep2>> || std::ratio_divide<typename U1::ratio, typename U2::ratio>::den == 1
  quantity<dimension_divide_t<D1, D2>, unit<dimension_divide_t<D1, D2>, std::ratio_divide<typename U1::ratio, typename U2::ratio>>, std::common_type_t<Rep1, Rep2>>
  constexpr operator/(const quantity<D1, U1, Rep1>& lhs,
                      const quantity<D2, U2, Rep2>& rhs)
  {
    using dim = dimension_divide_t<D1, D2>;
    using ret = quantity<dim, unit<dim, std::ratio_divide<typename U1::ratio, typename U2::ratio>>, std::common_type_t<Rep1, Rep2>>;
    return ret(lhs.count() / rhs.count());
  }

  template<Dimension D, Unit U, typename Rep1, typename Rep2>
  quantity<D, U, std::common_type_t<Rep1, Rep2>>
  constexpr operator%(const quantity<D, U, Rep1>& q,
                      const Rep2& v)
  {
    using ret = quantity<D, U, std::common_type_t<Rep1, Rep2>>;
    return ret(ret(q).count() % v);
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>
  constexpr operator%(const quantity<D, U1, Rep1>& lhs,
                      const quantity<D, U2, Rep2>& rhs)
  {
    using ret = std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>;
    return ret(ret(lhs).count() % ret(rhs).count());
  }

  // clang-format on

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  constexpr bool operator==(const quantity<D, U1, Rep1>& lhs, const quantity<D, U2, Rep2>& rhs)
  {
    using ct = std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>;
    return ct(lhs).count() == ct(rhs).count();
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  constexpr bool operator!=(const quantity<D, U1, Rep1>& lhs, const quantity<D, U2, Rep2>& rhs)
  {
    return !(lhs == rhs);
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  constexpr bool operator<(const quantity<D, U1, Rep1>& lhs, const quantity<D, U2, Rep2>& rhs)
  {
    using ct = std::common_type_t<quantity<D, U1, Rep1>, quantity<D, U2, Rep2>>;
    return ct(lhs).count() < ct(rhs).count();
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  constexpr bool operator<=(const quantity<D, U1, Rep1>& lhs, const quantity<D, U2, Rep2>& rhs)
  {
    return !(rhs < lhs);
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  constexpr bool operator>(const quantity<D, U1, Rep1>& lhs, const quantity<D, U2, Rep2>& rhs)
  {
    return rhs < lhs;
  }

  template<Dimension D, Unit U1, typename Rep1, Unit U2, typename Rep2>
  constexpr bool operator>=(const quantity<D, U1, Rep1>& lhs, const quantity<D, U2, Rep2>& rhs)
  {
    return !(lhs < rhs);
  }

}  // namespace units

namespace std {

  // todo: simplified
  template<units::Dimension D, units::Unit U1, typename Rep1, units::Unit U2, typename Rep2>
  struct common_type<units::quantity<D, U1, Rep1>, units::quantity<D, U2, Rep2>> {
    using type = units::quantity<D, units::unit<D, units::common_ratio_t<typename U1::ratio, typename U2::ratio>>,
                                 std::common_type_t<Rep1, Rep2>>;
  };

}  // namespace std