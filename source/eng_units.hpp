// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>

namespace lob {

template <typename E, E U, typename T>
class StrongT {
 public:
  explicit StrongT<E, U, T>(T value) : value_(value) {}
  StrongT<E, U, T>(const StrongT<E, U, T>& other) = default;
  StrongT<E, U, T>(StrongT<E, U, T>&& other) noexcept = default;
  ~StrongT() = default;
  StrongT<E, U, T>& operator=(const StrongT<E, U, T>& rhs) {
    if (this != &rhs) {
      value_ = rhs.value_;
    }
    return *this;
  }
  StrongT<E, U, T>& operator=(StrongT<E, U, T>&& rhs) noexcept {
    if (this != &rhs) {
      value_ = rhs.value_;
      rhs.value_ = 0;
    }
    return *this;
  }

  // Conversion operators
  explicit operator T() const { return value_; }

  template <E Other>
  operator StrongT<E, Other, T>() const;  // NOLINT allow implicit conversions

  // Arithmetic operators
  StrongT<E, U, T> operator+(const StrongT<E, U, T>& rhs) const {
    return StrongT<E, U, T>(value_ + rhs.value_);
  }
  StrongT<E, U, T> operator-(const StrongT<E, U, T>& rhs) const {
    return StrongT<E, U, T>(value_ - rhs.value_);
  }
  StrongT<E, U, T> operator*(const StrongT<E, U, T>& rhs) const {
    return StrongT<E, U, T>(value_ * rhs.value_);
  }
  StrongT<E, U, T> operator/(const StrongT<E, U, T>& rhs) const {
    return StrongT<E, U, T>(value_ / rhs.value_);
  }
  StrongT<E, U, T> operator+(const T& rhs) const {
    return StrongT<E, U, T>(value_ + rhs);
  }
  StrongT<E, U, T> operator-(const T& rhs) const {
    return StrongT<E, U, T>(value_ - rhs);
  }
  StrongT<E, U, T> operator*(const T& rhs) const {
    return StrongT<E, U, T>(value_ * rhs);
  }
  StrongT<E, U, T> operator/(const T& rhs) const {
    return StrongT<E, U, T>(value_ / rhs);
  }

  // Modulo operator
  StrongT<E, U, T> operator%(const StrongT<E, U, T>& rhs) const {
    return StrongT<E, U, T>(value_ % rhs.value_);
  }
  StrongT<E, U, T> operator%(const T& rhs) const {
    return StrongT<E, U, T>(value_ % rhs);
  }

  // Arithmetic assignment operators
  StrongT<E, U, T>& operator+=(const StrongT<E, U, T>& rhs) {
    value_ += rhs.value_;
    return *this;
  }
  StrongT<E, U, T>& operator-=(const StrongT<E, U, T>& rhs) {
    value_ -= rhs.value_;
    return *this;
  }
  StrongT<E, U, T>& operator*=(const StrongT<E, U, T>& rhs) {
    value_ *= rhs.value_;
    return *this;
  }
  StrongT<E, U, T>& operator/=(const StrongT<E, U, T>& rhs) {
    value_ /= rhs.value_;
    return *this;
  }
  StrongT<E, U, T>& operator+=(const T& rhs) {
    value_ += rhs;
    return *this;
  }
  StrongT<E, U, T>& operator-=(const T& rhs) {
    value_ -= rhs;
    return *this;
  }
  StrongT<E, U, T>& operator*=(const T& rhs) {
    value_ *= rhs;
    return *this;
  }
  StrongT<E, U, T>& operator/=(const T& rhs) {
    value_ /= rhs;
    return *this;
  }

  // Increment operators
  StrongT<E, U, T>& operator++() {
    ++value_;
    return *this;
  }
  StrongT<E, U, T> operator++(int) & {  // NOLINT
    StrongT<E, U, T> temp(*this);
    ++value_;
    return temp;
  }
  StrongT<E, U, T>& operator--() {
    --value_;
    return *this;
  }
  StrongT<E, U, T> operator--(int) & {  //  NOLINT
    StrongT<E, U, T> temp(*this);
    --value_;
    return temp;
  }

  // Comparison operators
  bool operator==(const StrongT<E, U, T>& rhs) const {
    return value_ == rhs.value_;
  }
  bool operator!=(const StrongT<E, U, T>& rhs) const {
    return value_ != rhs.value_;
  }
  bool operator<(const StrongT<E, U, T>& rhs) const {
    return value_ < rhs.value_;
  }
  bool operator>(const StrongT<E, U, T>& rhs) const {
    return value_ > rhs.value_;
  }
  bool operator<=(const StrongT<E, U, T>& rhs) const {
    return value_ <= rhs.value_;
  }
  bool operator>=(const StrongT<E, U, T>& rhs) const {
    return value_ >= rhs.value_;
  }

  // Specialized std functions
  friend bool isnan(const StrongT& st) {  // NOLINT name styling
    return std::isnan(st.value_);
  }

  friend StrongT sqrt(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::sqrt(st.value_));
  }

  friend StrongT pow(const StrongT& base,  // NOLINT name styling
                     double exponent) {
    return StrongT(std::pow(base.value_, exponent));
  }

  friend StrongT pow(const StrongT& base,  // NOLINT name styling
                     const StrongT& exponent) {
    return StrongT(std::pow(base.value_, exponent.value_));
  }

  friend StrongT sin(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::sin(st.value_));
  }

  friend StrongT cos(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::cos(st.value_));
  }

  friend StrongT tan(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::tan(st.value_));
  }

  friend StrongT asin(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::sin(st.value_));
  }

  friend StrongT acos(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::cos(st.value_));
  }

  friend StrongT atan(const StrongT& st) {  // NOLINT name styling
    return StrongT(std::tan(st.value_));
  }

  friend StrongT min(const StrongT& a,  // NOLINT name styling
                     const StrongT& b) {
    return StrongT(std::min(a.value_, b.value_));
  }

  friend StrongT max(const StrongT& a,  // NOLINT name styling
                     const StrongT& b) {
    return StrongT(std::max(a.value_, b.value_));
  }

  T Value() const { return value_; }

 private:
  T value_;
  static_assert(std::is_enum<E>::value, "U must be an enum type");
  static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
};

enum class Acceleration : uint8_t { kFeetPerSecondSq };
using FpsSqT = StrongT<Acceleration, Acceleration::kFeetPerSecondSq, double>;

enum class Angle : uint8_t { kDegrees, kRadians, kMoa };
using DegreesT = StrongT<Angle, Angle::kDegrees, double>;
using RadiansT = StrongT<Angle, Angle::kRadians, double>;
using MoaT = StrongT<Angle, Angle::kMoa, double>;

template <>
template <>
DegreesT::operator RadiansT() const;

template <>
template <>
DegreesT::operator MoaT() const;

template <>
template <>
RadiansT::operator DegreesT() const;

template <>
template <>
RadiansT::operator MoaT() const;

template <>
template <>
MoaT::operator DegreesT() const;

template <>
template <>
MoaT::operator RadiansT() const;

enum class Area : uint8_t { kSquareFeet };
using SqFtT = StrongT<Area, Area::kSquareFeet, double>;

enum class Density : uint8_t { kPoundsPerCubicFoot };
using LbsPerCuFtT = StrongT<Density, Density::kPoundsPerCubicFoot, double>;

enum class Energy : uint8_t { kFootPounds, kJoules };
using FtLbsT = StrongT<Energy, Energy::kFootPounds, double>;
using JouleT = StrongT<Energy, Energy::kJoules, double>;

template <>
template <>
FtLbsT::operator JouleT() const;

template <>
template <>
JouleT::operator FtLbsT() const;

enum class Length : uint8_t { kInches, kFeet, kYards, kMillimeter, kMeter };
using InchT = StrongT<Length, Length::kInches, double>;
using FeetT = StrongT<Length, Length::kFeet, double>;
using YardT = StrongT<Length, Length::kYards, double>;
using MmT = StrongT<Length, Length::kMillimeter, double>;
using MeterT = StrongT<Length, Length::kMeter, double>;

template <>
template <>
InchT::operator FeetT() const;

template <>
template <>
YardT::operator FeetT() const;

template <>
template <>
MmT::operator FeetT() const;

template <>
template <>
MmT::operator InchT() const;

template <>
template <>
MeterT::operator FeetT() const;

template <>
template <>
FeetT::operator InchT() const;

template <>
template <>
FeetT::operator YardT() const;

enum class Pressure : uint8_t { kPsi, kInchesOfMercury, kPascal, kMillibar };
using InHgT = StrongT<Pressure, Pressure::kInchesOfMercury, double>;
using PsiT = StrongT<Pressure, Pressure::kPsi, double>;
using PaT = StrongT<Pressure, Pressure::kPascal, double>;
using MillibarT = StrongT<Pressure, Pressure::kMillibar, double>;

template <>
template <>
PsiT::operator InHgT() const;

template <>
template <>
PaT::operator InHgT() const;

template <>
template <>
MillibarT::operator InHgT() const;

enum class Mass : uint8_t { kGrains, kPounds, kSlugs, kGrams, kKilograms };
using GrainT = StrongT<Mass, Mass::kGrains, double>;
using LbsT = StrongT<Mass, Mass::kPounds, double>;
using SlugT = StrongT<Mass, Mass::kSlugs, double>;
using GramT = StrongT<Mass, Mass::kGrams, double>;
using KgT = StrongT<Mass, Mass::kKilograms, double>;

template <>
template <>
GrainT::operator LbsT() const;

template <>
template <>
LbsT::operator GrainT() const;

template <>
template <>
LbsT::operator SlugT() const;

template <>
template <>
GrainT::operator SlugT() const;

template <>
template <>
GramT::operator LbsT() const;

template <>
template <>
KgT::operator LbsT() const;

enum class SectionalDensity : uint8_t {
  kPoundsMassPerSquareInch,
  kKilogramsPerSquareMeter
};
using PmsiT = StrongT<SectionalDensity,
                      SectionalDensity::kPoundsMassPerSquareInch, double>;
using KgsmT = StrongT<SectionalDensity,
                      SectionalDensity::kKilogramsPerSquareMeter, double>;

template <>
template <>
KgsmT::operator PmsiT() const;

template <>
template <>
PmsiT::operator KgsmT() const;

enum class Speed : uint8_t {
  kFeetPerSecond,
  kMilesPerHour,
  kMeterPerSecond,
  kKilometerPerHour,
  kKnot
};

using FpsT = StrongT<Speed, Speed::kFeetPerSecond, double>;
using MphT = StrongT<Speed, Speed::kMilesPerHour, double>;
using MpsT = StrongT<Speed, Speed::kMeterPerSecond, double>;
using KphT = StrongT<Speed, Speed::kKilometerPerHour, double>;
using KnT = StrongT<Speed, Speed::kKnot, double>;

template <>
template <>
MphT::operator FpsT() const;

template <>
template <>
MpsT::operator FpsT() const;

template <>
template <>
KphT::operator FpsT() const;

template <>
template <>
KnT::operator FpsT() const;

enum class Temperature : uint8_t { kDegreesC, kDegreesF, kDegreesK, kDegreesR };
using DegCT = StrongT<Temperature, Temperature::kDegreesC, double>;
using DegFT = StrongT<Temperature, Temperature::kDegreesF, double>;
using DegKT = StrongT<Temperature, Temperature::kDegreesK, double>;
using DegRT = StrongT<Temperature, Temperature::kDegreesR, double>;

template <>
template <>
DegCT::operator DegFT() const;

template <>
template <>
DegFT::operator DegCT() const;

template <>
template <>
DegFT::operator DegRT() const;

template <>
template <>
DegKT::operator DegRT() const;

enum class Time : uint8_t { kMicroseconds, kMilliseconds, kSeconds };
using UsecT = StrongT<Time, Time::kMicroseconds, double>;
using MsecT = StrongT<Time, Time::kMilliseconds, double>;
using SecT = StrongT<Time, Time::kSeconds, double>;

template <>
template <>
UsecT::operator SecT() const;

template <>
template <>
SecT::operator UsecT() const;

template <>
template <>
MsecT::operator SecT() const;

template <>
template <>
SecT::operator MsecT() const;

enum class TwistRate : uint8_t { kInchesPerTurn, kMillimetersPerTurn };
using InchPerTwistT = StrongT<TwistRate, TwistRate::kInchesPerTurn, double>;
using MmPerTwistT = StrongT<TwistRate, TwistRate::kMillimetersPerTurn, double>;

template <>
template <>
MmPerTwistT::operator InchPerTwistT() const;

}  // namespace lob

namespace std {

// Static analyzers don't like modifications to std but this is just to allow
// specializations for our custom type.

template <typename E, E U, typename T>
bool isnan(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return isnan(st.Value());
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> sqrt(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::sqrt(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> pow(const lob::StrongT<E, U, T>& base,  // NOLINT
                          const lob::StrongT<E, U, T>& exponent) {
  return lob::StrongT<E, U, T>(std::pow(base.Value(), exponent.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> pow(const lob::StrongT<E, U, T>& base,  // NOLINT
                          double exponent) {
  return lob::StrongT<E, U, T>(std::pow(base.Value(), exponent));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> sin(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::sin(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> cos(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::cos(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> tan(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::tan(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> asin(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::asin(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> acos(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::acos(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> atan(const lob::StrongT<E, U, T>& st) {  // NOLINT
  return lob::StrongT<E, U, T>(std::atan(st.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> min(const lob::StrongT<E, U, T>& a,  // NOLINT
                          const lob::StrongT<E, U, T>& b) {
  return lob::StrongT<E, U, T>(std::min(a.Value(), b.Value()));
}

template <typename E, E U, typename T>
lob::StrongT<E, U, T> max(const lob::StrongT<E, U, T>& a,  // NOLINT
                          const lob::StrongT<E, U, T>& b) {
  return lob::StrongT<E, U, T>(std::max(a.Value(), b.Value()));
}

}  // namespace std

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.