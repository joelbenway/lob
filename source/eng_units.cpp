// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "eng_units.hpp"

#include "constants.hpp"

namespace lob {

namespace convert {
// Angle
constexpr double kRadiansPerDegree = kPi / 180.0;
constexpr double kMoaPerDegree = 60.0;
constexpr double kMoaPerRadian = kMoaPerDegree / kRadiansPerDegree;
// Energy
constexpr double kJoulesPerFtLb = 1.3558179483;
// Length
constexpr double kInchPerFoot = 12.0;
constexpr double kFeetPerYard = 3;
constexpr double kMeterPerFoot = 0.3048;
constexpr double kMmPerFoot = kMeterPerFoot * 1000;
constexpr double kInchPerMm = kInchPerFoot / kMmPerFoot;
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

}  // namespace convert

// Angle

template <>
template <>
DegreesT::operator RadiansT() const {
  return RadiansT(value_ * convert::kRadiansPerDegree);
}

template <>
template <>
DegreesT::operator MoaT() const {
  return MoaT(value_ * convert::kMoaPerDegree);
}

template <>
template <>
RadiansT::operator DegreesT() const {
  return DegreesT(value_ / convert::kRadiansPerDegree);
}

template <>
template <>
RadiansT::operator MoaT() const {
  return MoaT(value_ * convert::kMoaPerRadian);
}

template <>
template <>
MoaT::operator DegreesT() const {
  return DegreesT(value_ / convert::kMoaPerDegree);
}

template <>
template <>
MoaT::operator RadiansT() const {
  return RadiansT(value_ / convert::kMoaPerRadian);
}

// Energy

template <>
template <>
FtLbsT::operator JouleT() const {
  return JouleT(value_ * convert::kJoulesPerFtLb);
}

template <>
template <>
JouleT::operator FtLbsT() const {
  return FtLbsT(value_ / convert::kJoulesPerFtLb);
}

// Length

template <>
template <>
InchT::operator FeetT() const {
  return FeetT(value_ / convert::kInchPerFoot);
}

template <>
template <>
YardT::operator FeetT() const {
  return FeetT(value_ * convert::kFeetPerYard);
}

template <>
template <>
MmT::operator FeetT() const {
  return FeetT(value_ / convert::kMmPerFoot);
}

template <>
template <>
MmT::operator InchT() const {
  return InchT(value_ * convert::kInchPerMm);
}

template <>
template <>
MeterT::operator FeetT() const {
  return FeetT(value_ / convert::kMeterPerFoot);
}

template <>
template <>
FeetT::operator InchT() const {
  return InchT(value_ * convert::kInchPerFoot);
}

template <>
template <>
FeetT::operator YardT() const {
  return YardT(value_ / convert::kFeetPerYard);
}

// Pressure

template <>
template <>
PsiT::operator InHgT() const {
  return InHgT(value_ * convert::kInHgPerPsi);
}

template <>
template <>
PaT::operator InHgT() const {
  return InHgT(value_ * convert::kInHgPerPa);
}

template <>
template <>
MillibarT::operator InHgT() const {
  return InHgT(value_ * convert::kInHgPerMillibar);
}

// Mass

template <>
template <>
GrainT::operator LbsT() const {
  return LbsT(value_ / convert::kGrainsPerLb);
}

template <>
template <>
LbsT::operator GrainT() const {
  return GrainT(value_ * convert::kGrainsPerLb);
}

template <>
template <>
LbsT::operator SlugT() const {
  return SlugT(value_ / convert::kLbsPerSlug);
}

template <>
template <>
GrainT::operator SlugT() const {
  return SlugT(value_ / (convert::kLbsPerSlug * convert::kGrainsPerLb));
}

template <>
template <>
GramT::operator LbsT() const {
  return LbsT(value_ * convert::kLbsPerGram);
}

template <>
template <>
KgT::operator LbsT() const {
  return LbsT(value_ * convert::kLbsPerKg);
}

// Sectional Density

template <>
template <>
KgsmT::operator PmsiT() const {
  return PmsiT(value_ * convert::kLbsmPerSqInPerKgPerSqM);
}

template <>
template <>
PmsiT::operator KgsmT() const {
  return KgsmT(value_ / convert::kLbsmPerSqInPerKgPerSqM);
}

// Speed

template <>
template <>
MphT::operator FpsT() const {
  return FpsT(value_ * convert::kFpsPerMph);
}

template <>
template <>
MpsT::operator FpsT() const {
  return FpsT(value_ / convert::kMeterPerFoot);
}

template <>
template <>
KphT::operator FpsT() const {
  return FpsT(value_ * convert::kFpsPerKph);
}

template <>
template <>
KnT::operator FpsT() const {
  return FpsT(value_ * convert::kFpsPerKn);
}

// Temperature

template <>
template <>
DegCT::operator DegFT() const {
  return DegFT(value_ * convert::kDegFPerDegC + convert::kFreezePointDegF);
}

template <>
template <>
DegFT::operator DegCT() const {
  return DegCT((value_ - convert::kFreezePointDegF) / convert::kDegFPerDegC);
}

template <>
template <>
DegFT::operator DegRT() const {
  return DegRT(value_ - convert::kAbsoluteZeroDegF);
}

template <>
template <>
DegKT::operator DegRT() const {
  return DegRT(value_ * convert::kDegFPerDegC);
}

// Time

template <>
template <>
UsecT::operator SecT() const {
  return SecT(value_ / convert::kUsecPerSec);
}

template <>
template <>
SecT::operator UsecT() const {
  return UsecT(value_ * convert::kUsecPerSec);
}

template <>
template <>
MsecT::operator SecT() const {
  return SecT(value_ / convert::kMsecPerSec);
}

template <>
template <>
SecT::operator MsecT() const {
  return MsecT(value_ * convert::kMsecPerSec);
}

// TwistRate

template <>
template <>
MmPerTwistT::operator InchPerTwistT() const {
  return InchPerTwistT(value_ * convert::kInchPerMm);
}

}  // namespace lob

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