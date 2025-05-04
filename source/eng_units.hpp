// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>

#include "constants.hpp"
#include "helpers.hpp"

namespace lob {

template <typename E, E U, typename T>
class StrongT {
 public:
  constexpr explicit StrongT(T value) : value_(value) {}
  template <E OtherUnit>
  constexpr explicit StrongT(const StrongT<E, OtherUnit, T>& other,
                             T conversion_factor)
      : value_(other.Value() * conversion_factor) {
    static_assert(std::is_same<E, decltype(OtherUnit)>::value,
                  "Units must share the same enum type");
    static_assert(U != OtherUnit, "Units must be of different types");
  }
  template <E OtherUnit>
  constexpr explicit StrongT(const StrongT<E, OtherUnit, T>& other,
                             const StrongT<E, OtherUnit, T>& conversion_factor)
      : value_(other.Value() * conversion_factor.Value()) {
    static_assert(std::is_same<E, decltype(OtherUnit)>::value,
                  "Units must share the same enum type");
    static_assert(U != OtherUnit, "Units must be of different types");
  }
  constexpr StrongT(const StrongT& other) = default;
  constexpr StrongT(StrongT&& other) noexcept = default;
  ~StrongT() = default;
  constexpr StrongT& operator=(const StrongT& rhs) {
    if (this != &rhs) {
      value_ = rhs.value_;
    }
    return *this;
  }
  constexpr StrongT& operator=(StrongT&& rhs) noexcept {
    if (this != &rhs) {
      value_ = rhs.value_;
      rhs.value_ = 0;
    }
    return *this;
  }

  // Conversion operators
  constexpr explicit operator T() const { return value_; }

  template <E Other>
  // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
  constexpr operator StrongT<E, Other, T>() const;

  // Arithmetic operators
  constexpr StrongT operator+(const StrongT& rhs) const {
    return StrongT(value_ + rhs.value_);
  }
  constexpr StrongT operator-(const StrongT& rhs) const {
    return StrongT(value_ - rhs.value_);
  }
  constexpr StrongT operator*(const StrongT& rhs) const {
    return StrongT(value_ * rhs.value_);
  }
  constexpr StrongT operator/(const StrongT& rhs) const {
    return StrongT(value_ / rhs.value_);
  }
  constexpr StrongT operator+(const T& rhs) const {
    return StrongT(value_ + rhs);
  }
  constexpr StrongT operator-(const T& rhs) const {
    return StrongT(value_ - rhs);
  }
  constexpr StrongT operator*(const T& rhs) const {
    return StrongT(value_ * rhs);
  }
  constexpr StrongT operator/(const T& rhs) const {
    return StrongT(value_ / rhs);
  }

  // Modulo operator
  constexpr StrongT operator%(const StrongT& rhs) const {
    return StrongT(Modulo(value_, rhs.value_));
  }
  constexpr StrongT operator%(const T& rhs) const {
    return StrongT(Modulo(value_, rhs));
  }

  // Arithmetic assignment operators
  constexpr StrongT& operator+=(const StrongT& rhs) {
    value_ += rhs.value_;
    return *this;
  }
  constexpr StrongT& operator-=(const StrongT& rhs) {
    value_ -= rhs.value_;
    return *this;
  }
  constexpr StrongT& operator*=(const StrongT& rhs) {
    value_ *= rhs.value_;
    return *this;
  }
  constexpr StrongT& operator/=(const StrongT& rhs) {
    value_ /= rhs.value_;
    return *this;
  }
  constexpr StrongT& operator+=(const T& rhs) {
    value_ += rhs;
    return *this;
  }
  constexpr StrongT& operator-=(const T& rhs) {
    value_ -= rhs;
    return *this;
  }
  constexpr StrongT& operator*=(const T& rhs) {
    value_ *= rhs;
    return *this;
  }
  constexpr StrongT& operator/=(const T& rhs) {
    value_ /= rhs;
    return *this;
  }

  // Increment operators
  constexpr StrongT& operator++() {
    ++value_;
    return *this;
  }
  constexpr StrongT operator++(int) & {
    StrongT temp(*this);
    ++value_;
    return temp;
  }
  constexpr StrongT& operator--() {
    --value_;
    return *this;
  }
  constexpr StrongT operator--(int) & {
    StrongT temp(*this);
    --value_;
    return temp;
  }

  // Comparison operators
  constexpr bool operator==(const StrongT& rhs) const {
    return AreEqual(value_, rhs.value_);
  }
  constexpr bool operator!=(const StrongT& rhs) const {
    return !AreEqual(value_, rhs.value_);
  }
  constexpr bool operator<(const StrongT& rhs) const {
    return value_ < rhs.value_;
  }
  constexpr bool operator>(const StrongT& rhs) const {
    return value_ > rhs.value_;
  }
  constexpr bool operator<=(const StrongT& rhs) const {
    return value_ <= rhs.value_;
  }
  constexpr bool operator>=(const StrongT& rhs) const {
    return value_ >= rhs.value_;
  }

  // Specialized std functions
  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr bool isnan(const StrongT& st) noexcept {
    return std::isnan(st.value_);
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT sqrt(const StrongT& st) noexcept {
    return StrongT(std::sqrt(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT pow(const StrongT& base, double exponent) noexcept {
    return StrongT(std::pow(base.value_, exponent));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT pow(const StrongT& base,
                               const StrongT& exponent) noexcept {
    return StrongT(std::pow(base.value_, exponent.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT sin(const StrongT& st) noexcept {
    return StrongT(std::sin(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT cos(const StrongT& st) noexcept {
    return StrongT(std::cos(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT tan(const StrongT& st) noexcept {
    return StrongT(std::tan(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT asin(const StrongT& st) noexcept {
    return StrongT(std::sin(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT acos(const StrongT& st) noexcept {
    return StrongT(std::cos(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT atan(const StrongT& st) noexcept {
    return StrongT(std::tan(st.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT min(const StrongT& a, const StrongT& b) noexcept {
    return StrongT(std::min(a.value_, b.value_));
  }

  // NOLINTNEXTLINE(readability-identifier-naming)
  friend constexpr StrongT max(const StrongT& a, const StrongT& b) noexcept {
    return StrongT(std::max(a.value_, b.value_));
  }

  template <typename TT = T>
  constexpr auto IsNaN() const ->
      typename std::enable_if_t<std::is_floating_point<TT>::value, bool> {
    return std::isnan(value_);
  }
  template <typename TT = T>
  constexpr auto IsNaN() const ->
      typename std::enable_if_t<!std::is_floating_point<TT>::value, bool> {
    return false;
  }
  constexpr StrongT Inverse() const { return StrongT(T(1) / value_); }
  constexpr T Value() const { return value_; }
  constexpr float Float() const { return static_cast<float>(value_); }
  constexpr uint32_t U32() const {
    return value_ < 0 ? 0U : static_cast<uint32_t>(std::round(value_));
  }
  constexpr uint16_t U16() const {
    return value_ < 0 ? 0U : static_cast<uint16_t>(std::round(value_));
  }

 private:
  T value_;
  static_assert(std::is_enum<E>::value, "U must be an enum type");
  static_assert(std::is_arithmetic<T>::value, "T must be an arithmetic type");
};

namespace convert {
// Angle
constexpr double kRadiansPerDegree = kPi / 180.0;
constexpr double kMoaPerDegree = 60.0;
constexpr double kMoaPerRadian = kMoaPerDegree / kRadiansPerDegree;
constexpr double kMilPerRadian = 1'000;
constexpr double kMilPerDegree = kMilPerRadian * kRadiansPerDegree;
constexpr double kMoaPerMil = kMoaPerRadian / kMilPerRadian;
constexpr double kIphyPerMoa = 1.047;
// Energy
constexpr double kJoulesPerFtLb = 1.3558179483;
// Length
constexpr double kInchPerFoot = 12.0;
constexpr double kFeetPerYard = 3;
constexpr double kMeterPerFoot = 0.3048;
constexpr double kMmPerFoot = kMeterPerFoot * 1000;
constexpr double kCmPerFoot = kMeterPerFoot * 100;
constexpr double kInchPerMm = kInchPerFoot / kMmPerFoot;
constexpr double kInchPerCm = kInchPerMm * 10;
// Pressure
constexpr double kInHgPerPa = 0.000295299801647;
constexpr double kInHgPerPsi = 2.03602128864;
constexpr double kInHgPerMillibar = kInHgPerPa * 100;
// Mass
constexpr double kGrainsPerLb = 7000.0;
constexpr double kLbsPerSlug = 32.17405;
constexpr double kLbsPerKg = 2.204623;
constexpr double kLbsPerGram = kLbsPerKg / 1000;
// Sectional Density
constexpr double kLbsmPerSqInPerKgPerSqM = 703.069579639;
// Speed
constexpr double kFpsPerMph = 1.46666667;
constexpr double kFpsPerKph = 0.91134442;
constexpr double kFpsPerKn = 1.6878099;
// Time
constexpr double kMsecPerSec = 1E3;
constexpr double kUsecPerSec = 1E6;
// Temperature
constexpr double kDegFPerDegC = 1.8;
constexpr double kFreezePointDegF = 32.0;
constexpr double kAbsoluteZeroDegF = -459.67;
constexpr double kAbsoluteZeroDegC =
    (kAbsoluteZeroDegF - kFreezePointDegF) / kDegFPerDegC;
}  // namespace convert

enum class Acceleration : uint8_t { kFeetPerSecondSq };
using FpsSqT = StrongT<Acceleration, Acceleration::kFeetPerSecondSq, double>;

enum class Angle : uint8_t { kDegrees, kRadians, kMoa, kMil, kIphy };
using DegreesT = StrongT<Angle, Angle::kDegrees, double>;
using RadiansT = StrongT<Angle, Angle::kRadians, double>;
using MoaT = StrongT<Angle, Angle::kMoa, double>;
using MilT = StrongT<Angle, Angle::kMil, double>;
using IphyT = StrongT<Angle, Angle::kIphy, double>;

template <>
template <>
constexpr DegreesT::operator RadiansT() const {
  return RadiansT(Value() * convert::kRadiansPerDegree);
}

template <>
template <>
constexpr DegreesT::operator MoaT() const {
  return MoaT(Value() * convert::kMoaPerDegree);
}

template <>
template <>
constexpr DegreesT::operator MilT() const {
  return MilT(Value() * convert::kMilPerDegree);
}

template <>
template <>
constexpr DegreesT::operator IphyT() const {
  return IphyT(Value() * convert::kMoaPerDegree * convert::kIphyPerMoa);
}

template <>
template <>
constexpr RadiansT::operator DegreesT() const {
  return DegreesT(Value() / convert::kRadiansPerDegree);
}

template <>
template <>
constexpr RadiansT::operator MoaT() const {
  return MoaT(Value() * convert::kMoaPerRadian);
}

template <>
template <>
constexpr RadiansT::operator MilT() const {
  return MilT(Value() * convert::kMilPerRadian);
}

template <>
template <>
constexpr RadiansT::operator IphyT() const {
  return IphyT(Value() * convert::kMoaPerRadian * convert::kIphyPerMoa);
}

template <>
template <>
constexpr MoaT::operator DegreesT() const {
  return DegreesT(Value() / convert::kMoaPerDegree);
}

template <>
template <>
constexpr MoaT::operator RadiansT() const {
  return RadiansT(Value() / convert::kMoaPerRadian);
}

template <>
template <>
constexpr MoaT::operator MilT() const {
  return MilT(Value() / convert::kMoaPerMil);
}

template <>
template <>
constexpr MoaT::operator IphyT() const {
  return IphyT(Value() * convert::kIphyPerMoa);
}

template <>
template <>
constexpr MilT::operator DegreesT() const {
  return DegreesT(Value() / convert::kMilPerDegree);
}

template <>
template <>
constexpr MilT::operator RadiansT() const {
  return RadiansT(Value() / convert::kMilPerRadian);
}

template <>
template <>
constexpr MilT::operator MoaT() const {
  return MoaT(Value() * convert::kMoaPerMil);
}

template <>
template <>
constexpr MilT::operator IphyT() const {
  return IphyT(Value() * convert::kMoaPerMil * convert::kIphyPerMoa);
}

template <>
template <>
constexpr IphyT::operator DegreesT() const {
  return DegreesT(Value() / convert::kIphyPerMoa / convert::kMoaPerDegree);
}

template <>
template <>
constexpr IphyT::operator MoaT() const {
  return MoaT(Value() / convert::kIphyPerMoa);
}

template <>
template <>
constexpr IphyT::operator RadiansT() const {
  return RadiansT(Value() / convert::kIphyPerMoa / convert::kMoaPerRadian);
}

template <>
template <>
constexpr IphyT::operator MilT() const {
  return MilT(Value() / convert::kIphyPerMoa / convert::kMoaPerMil);
}

enum class Area : uint8_t { kSquareInches, kSquareFeet };
using SqInT = StrongT<Area, Area::kSquareInches, double>;
using SqFtT = StrongT<Area, Area::kSquareFeet, double>;

template <>
template <>
constexpr SqFtT::operator SqInT() const {
  return SqInT(Value() * convert::kInchPerFoot * convert::kInchPerFoot);
}

template <>
template <>
constexpr SqInT::operator SqFtT() const {
  return SqFtT(Value() / convert::kInchPerFoot / convert::kInchPerFoot);
}

enum class Density : uint8_t { kGrainsPerCubicInch, kPoundsPerCubicFoot };
using GrPerCuInT = StrongT<Density, Density::kGrainsPerCubicInch, double>;
using LbsPerCuFtT = StrongT<Density, Density::kPoundsPerCubicFoot, double>;

template <>
template <>
constexpr LbsPerCuFtT::operator GrPerCuInT() const {
  using convert::kInchPerFoot;
  return GrPerCuInT(Value() * convert::kGrainsPerLb /
                    (kInchPerFoot * kInchPerFoot * kInchPerFoot));
}

template <>
template <>
constexpr GrPerCuInT::operator LbsPerCuFtT() const {
  using convert::kInchPerFoot;
  return LbsPerCuFtT(Value() / convert::kGrainsPerLb *
                     (kInchPerFoot * kInchPerFoot * kInchPerFoot));
}

enum class Energy : uint8_t { kFootPounds, kJoules };
using FtLbsT = StrongT<Energy, Energy::kFootPounds, double>;
using JouleT = StrongT<Energy, Energy::kJoules, double>;

template <>
template <>
constexpr FtLbsT::operator JouleT() const {
  return JouleT(Value() * convert::kJoulesPerFtLb);
}

template <>
template <>
constexpr JouleT::operator FtLbsT() const {
  return FtLbsT(Value() / convert::kJoulesPerFtLb);
}

enum class Frequency : uint8_t { kHz };

using HzT = StrongT<Frequency, Frequency::kHz, double>;

enum class Length : uint8_t {
  kInches,
  kFeet,
  kYards,
  kMillimeter,
  kCentimeter,
  kMeter,
  kCaliber
};
using InchT = StrongT<Length, Length::kInches, double>;
using FeetT = StrongT<Length, Length::kFeet, double>;
using YardT = StrongT<Length, Length::kYards, double>;
using MmT = StrongT<Length, Length::kMillimeter, double>;
using CmT = StrongT<Length, Length::kCentimeter, double>;
using MeterT = StrongT<Length, Length::kMeter, double>;

// Caliber is a relative length and cannot convert to other units.
using CaliberT = StrongT<Length, Length::kCaliber, double>;

template <>
template <>
constexpr InchT::operator FeetT() const {
  return FeetT(Value() / convert::kInchPerFoot);
}

template <>
template <>
constexpr InchT::operator MmT() const {
  return MmT(Value() / convert::kInchPerMm);
}

template <>
template <>
constexpr InchT::operator CmT() const {
  return CmT(Value() / convert::kInchPerCm);
}

template <>
template <>
constexpr YardT::operator InchT() const {
  return InchT(Value() * convert::kFeetPerYard * convert::kInchPerFoot);
}

template <>
template <>
constexpr YardT::operator FeetT() const {
  return FeetT(Value() * convert::kFeetPerYard);
}

template <>
template <>
constexpr YardT::operator MeterT() const {
  return MeterT(Value() * convert::kFeetPerYard * convert::kMeterPerFoot);
}

template <>
template <>
constexpr MmT::operator InchT() const {
  return InchT(Value() * convert::kInchPerMm);
}
template <>
template <>
constexpr MmT::operator FeetT() const {
  return FeetT(Value() / convert::kMmPerFoot);
}

template <>
template <>
constexpr CmT::operator InchT() const {
  return InchT(Value() * convert::kInchPerCm);
}

template <>
template <>
constexpr CmT::operator FeetT() const {
  return FeetT(Value() / convert::kCmPerFoot);
}

template <>
template <>
constexpr MeterT::operator InchT() const {
  return InchT(Value() / convert::kMeterPerFoot * convert::kInchPerFoot);
}

template <>
template <>
constexpr MeterT::operator FeetT() const {
  return FeetT(Value() / convert::kMeterPerFoot);
}

template <>
template <>
constexpr MeterT::operator YardT() const {
  return YardT(Value() / convert::kMeterPerFoot / convert::kFeetPerYard);
}

template <>
template <>
constexpr FeetT::operator InchT() const {
  return InchT(Value() * convert::kInchPerFoot);
}

template <>
template <>
constexpr FeetT::operator YardT() const {
  return YardT(Value() / convert::kFeetPerYard);
}

template <>
template <>
constexpr FeetT::operator MmT() const {
  return MmT(Value() * convert::kMmPerFoot);
}

template <>
template <>
constexpr FeetT::operator CmT() const {
  return CmT(Value() * convert::kCmPerFoot);
}

template <>
template <>
constexpr FeetT::operator MeterT() const {
  return MeterT(Value() * convert::kMeterPerFoot);
}

enum class Pressure : uint8_t { kPsi, kInchesOfMercury, kPascal, kMillibar };
using InHgT = StrongT<Pressure, Pressure::kInchesOfMercury, double>;
using PsiT = StrongT<Pressure, Pressure::kPsi, double>;
using PaT = StrongT<Pressure, Pressure::kPascal, double>;
using MbarT = StrongT<Pressure, Pressure::kMillibar, double>;

template <>
template <>
constexpr InHgT::operator PsiT() const {
  return PsiT(Value() / convert::kInHgPerPsi);
}

template <>
template <>
constexpr InHgT::operator PaT() const {
  return PaT(Value() / convert::kInHgPerPa);
}

template <>
template <>
constexpr InHgT::operator MbarT() const {
  return MbarT(Value() / convert::kInHgPerMillibar);
}

template <>
template <>
constexpr PsiT::operator InHgT() const {
  return InHgT(Value() * convert::kInHgPerPsi);
}

template <>
template <>
constexpr PaT::operator InHgT() const {
  return InHgT(Value() * convert::kInHgPerPa);
}

template <>
template <>
constexpr MbarT::operator InHgT() const {
  return InHgT(Value() * convert::kInHgPerMillibar);
}

enum class Mass : uint8_t { kGrains, kPounds, kSlugs, kGrams, kKilograms };
using GrainT = StrongT<Mass, Mass::kGrains, double>;
using LbsT = StrongT<Mass, Mass::kPounds, double>;
using SlugT = StrongT<Mass, Mass::kSlugs, double>;
using GramT = StrongT<Mass, Mass::kGrams, double>;
using KgT = StrongT<Mass, Mass::kKilograms, double>;

template <>
template <>
constexpr GrainT::operator LbsT() const {
  return LbsT(Value() / convert::kGrainsPerLb);
}

template <>
template <>
constexpr GrainT::operator SlugT() const {
  return SlugT(Value() / (convert::kLbsPerSlug * convert::kGrainsPerLb));
}

template <>
template <>
constexpr GrainT::operator GramT() const {
  return GramT(Value() / convert::kGrainsPerLb / convert::kLbsPerGram);
}

template <>
template <>
constexpr GrainT::operator KgT() const {
  return KgT(Value() / convert::kGrainsPerLb / convert::kLbsPerKg);
}

template <>
template <>
constexpr LbsT::operator GrainT() const {
  return GrainT(Value() * convert::kGrainsPerLb);
}

template <>
template <>
constexpr LbsT::operator SlugT() const {
  return SlugT(Value() / convert::kLbsPerSlug);
}

template <>
template <>
constexpr GramT::operator LbsT() const {
  return LbsT(Value() * convert::kLbsPerGram);
}

template <>
template <>
constexpr KgT::operator GrainT() const {
  return GrainT(Value() * convert::kLbsPerKg * convert::kGrainsPerLb);
}

template <>
template <>
constexpr KgT::operator LbsT() const {
  return LbsT(Value() * convert::kLbsPerKg);
}

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
constexpr KgsmT::operator PmsiT() const {
  return PmsiT(Value() * convert::kLbsmPerSqInPerKgPerSqM);
}

template <>
template <>
constexpr PmsiT::operator KgsmT() const {
  return KgsmT(Value() / convert::kLbsmPerSqInPerKgPerSqM);
}

enum class Speed : uint8_t {
  kMach,
  kFeetPerSecond,
  kMilesPerHour,
  kMeterPerSecond,
  kKilometerPerHour,
  kKnot
};

using MachT = StrongT<Speed, Speed::kMach, double>;
using FpsT = StrongT<Speed, Speed::kFeetPerSecond, double>;
using MphT = StrongT<Speed, Speed::kMilesPerHour, double>;
using MpsT = StrongT<Speed, Speed::kMeterPerSecond, double>;
using KphT = StrongT<Speed, Speed::kKilometerPerHour, double>;
using KnT = StrongT<Speed, Speed::kKnot, double>;

template <>
template <>
constexpr FpsT::operator MpsT() const {
  return MpsT(Value() * convert::kMeterPerFoot);
}

template <>
template <>
constexpr FpsT::operator MphT() const {
  return MphT(Value() / convert::kFpsPerMph);
}

template <>
template <>
constexpr MphT::operator FpsT() const {
  return FpsT(Value() * convert::kFpsPerMph);
}

template <>
template <>
constexpr MphT::operator KphT() const {
  return KphT(Value() * convert::kFpsPerMph / convert::kFpsPerKph);
}

template <>
template <>
constexpr MphT::operator KnT() const {
  return KnT(Value() * convert::kFpsPerMph / convert::kFpsPerKn);
}

template <>
template <>
constexpr MpsT::operator FpsT() const {
  return FpsT(Value() / convert::kMeterPerFoot);
}

template <>
template <>
constexpr KphT::operator FpsT() const {
  return FpsT(Value() * convert::kFpsPerKph);
}

template <>
template <>
constexpr KnT::operator FpsT() const {
  return FpsT(Value() * convert::kFpsPerKn);
}

enum class Temperature : uint8_t { kDegreesC, kDegreesF, kDegreesK, kDegreesR };
using DegCT = StrongT<Temperature, Temperature::kDegreesC, double>;
using DegFT = StrongT<Temperature, Temperature::kDegreesF, double>;
using DegKT = StrongT<Temperature, Temperature::kDegreesK, double>;
using DegRT = StrongT<Temperature, Temperature::kDegreesR, double>;

template <>
template <>
constexpr DegCT::operator DegFT() const {
  return DegFT((Value() * convert::kDegFPerDegC) + convert::kFreezePointDegF);
}

template <>
template <>
constexpr DegFT::operator DegCT() const {
  return DegCT((Value() - convert::kFreezePointDegF) / convert::kDegFPerDegC);
}

template <>
template <>
constexpr DegFT::operator DegRT() const {
  return DegRT(Value() - convert::kAbsoluteZeroDegF);
}

template <>
template <>
constexpr DegRT::operator DegFT() const {
  return DegFT(Value() + convert::kAbsoluteZeroDegF);
}

template <>
template <>
constexpr DegFT::operator DegKT() const {
  return DegKT(((Value() - convert::kFreezePointDegF) / convert::kDegFPerDegC) -
               convert::kAbsoluteZeroDegC);
}

template <>
template <>
constexpr DegKT::operator DegFT() const {
  return DegFT(
      ((Value() + convert::kAbsoluteZeroDegC) * convert::kDegFPerDegC) +
      convert::kFreezePointDegF);
}

template <>
template <>
constexpr DegRT::operator DegKT() const {
  return DegKT(
      ((Value() + convert::kAbsoluteZeroDegF - convert::kFreezePointDegF) /
       convert::kDegFPerDegC) -
      convert::kAbsoluteZeroDegC);
}

template <>
template <>
constexpr DegKT::operator DegRT() const {
  return DegRT(
      ((Value() + lob::convert::kAbsoluteZeroDegC) * convert::kDegFPerDegC) -
      lob::convert::kAbsoluteZeroDegF + convert::kFreezePointDegF);
}

enum class Time : uint8_t { kMicroseconds, kMilliseconds, kSeconds };
using UsecT = StrongT<Time, Time::kMicroseconds, double>;
using MsecT = StrongT<Time, Time::kMilliseconds, double>;
using SecT = StrongT<Time, Time::kSeconds, double>;

template <>
template <>
constexpr UsecT::operator SecT() const {
  return SecT(Value() / convert::kUsecPerSec);
}

template <>
template <>
constexpr SecT::operator UsecT() const {
  return UsecT(Value() * convert::kUsecPerSec);
}

template <>
template <>
constexpr MsecT::operator SecT() const {
  return SecT(Value() / convert::kMsecPerSec);
}

template <>
template <>
constexpr SecT::operator MsecT() const {
  return MsecT(Value() * convert::kMsecPerSec);
}

enum class TwistRate : uint8_t { kInchesPerTurn, kMillimetersPerTurn };
using InchPerTwistT = StrongT<TwistRate, TwistRate::kInchesPerTurn, double>;
using MmPerTwistT = StrongT<TwistRate, TwistRate::kMillimetersPerTurn, double>;

template <>
template <>
constexpr MmPerTwistT::operator InchPerTwistT() const {
  return InchPerTwistT(Value() * convert::kInchPerMm);
}

}  // namespace lob

namespace std {
// Static analyzers don't like modifications to std but this is just to allow
// specializations for our custom type.

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr bool isnan(const lob::StrongT<E, U, T>& st) {
  return isnan(st.Value());
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> sqrt(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::sqrt(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> pow(const lob::StrongT<E, U, T>& base,
                                    const lob::StrongT<E, U, T>& exponent) {
  return lob::StrongT<E, U, T>(std::pow(base.Value(), exponent.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> pow(const lob::StrongT<E, U, T>& base,
                                    double exponent) {
  return lob::StrongT<E, U, T>(std::pow(base.Value(), exponent));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> sin(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::sin(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> cos(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::cos(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> tan(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::tan(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> asin(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::asin(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> acos(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::acos(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> atan(const lob::StrongT<E, U, T>& st) {
  return lob::StrongT<E, U, T>(std::atan(st.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> min(const lob::StrongT<E, U, T>& a,
                                    const lob::StrongT<E, U, T>& b) {
  return lob::StrongT<E, U, T>(std::min(a.Value(), b.Value()));
}

template <typename E, E U, typename T>
// NOLINTNEXTLINE(cert-dcl58-cpp, readability-identifier-naming)
constexpr lob::StrongT<E, U, T> max(const lob::StrongT<E, U, T>& a,
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
