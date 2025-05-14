// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cassert>
#include <cmath>

#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.hpp"
#include "version.hpp"

namespace lob {

const char* Version() { return kProjectVersion; }

namespace {
constexpr double kHundredYardsInFeet = FeetT(YardT(100)).Value();
}  // namespace

// Angle
double MoaToMil(double value) { return MilT(MoaT(value)).Value(); }
double MoaToDeg(double value) { return DegreesT(MoaT(value)).Value(); }
double MoaToIphy(double value) { return IphyT(MoaT(value)).Value(); }
double MoaToInch(double value, double range_ft) {
  return IphyT(MoaT(value)).Value() * range_ft / kHundredYardsInFeet;
}

double MilToMoa(double value) { return MoaT(MilT(value)).Value(); }
double MilToDeg(double value) { return DegreesT(MilT(value)).Value(); }
double MilToIphy(double value) { return IphyT(MilT(value)).Value(); }
double MilToInch(double value, double range_ft) {
  return IphyT(MilT(value)).Value() * range_ft / kHundredYardsInFeet;
}

double DegToMoa(double value) { return MoaT(DegreesT(value)).Value(); }
double DegToMil(double value) { return MilT(DegreesT(value)).Value(); }

double InchToMoa(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return MoaT(IphyT(value / (range_ft / kHundredYardsInFeet))).Value();
}

double InchToMil(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return MilT(IphyT(value / (range_ft / kHundredYardsInFeet))).Value();
}

double InchToDeg(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return DegreesT(IphyT(value / (range_ft / kHundredYardsInFeet))).Value();
}

// Energy
double JToFtLbs(double value) { return FtLbsT(JouleT(value)).Value(); }
double FtLbsToJ(double value) { return JouleT(FtLbsT(value)).Value(); }

// Length
double MtoYd(double value) { return YardT(MeterT(value)).Value(); }
double YdToFt(double value) { return FeetT(YardT(value)).Value(); }
double MToFt(double value) { return FeetT(MeterT(value)).Value(); }
double FtToIn(double value) { return InchT(FeetT(value)).Value(); }
double MmToIn(double value) { return InchT(MmT(value)).Value(); }
double CmToIn(double value) { return InchT(CmT(value)).Value(); }
double YdToM(double value) { return MeterT(YardT(value)).Value(); }
double FtToM(double value) { return MeterT(FeetT(value)).Value(); }
double FtToYd(double value) { return YardT(FeetT(value)).Value(); }
double InToMm(double value) { return MmT(InchT(value)).Value(); }
double InToCm(double value) { return CmT(InchT(value)).Value(); }
double InToFt(double value) { return FeetT(InchT(value)).Value(); }

// Pressure
double PaToInHg(double value) { return InHgT(PaT(value)).Value(); }
double MbarToInHg(double value) { return InHgT(MbarT(value)).Value(); }
double PsiToInHg(double value) { return InHgT(PsiT(value)).Value(); }

// Mass
double LbsToGrain(double value) { return GrainT(LbsT(value)).Value(); }
double GToGrain(double value) { return GrainT(LbsT(GramT(value))).Value(); }
double KgToGrain(double value) { return GrainT(LbsT(KgT(value))).Value(); }

// Sectional Density / Ballistic Coefficient
double KgSqMToPmsi(double value) { return PmsiT(KgsmT(value)).Value(); }

// Speed
double FpsToMps(double value) { return MpsT(FpsT(value)).Value(); }
double MpsToFps(double value) { return FpsT(MpsT(value)).Value(); }
double KphToMph(double value) { return MphT(FpsT(KphT(value))).Value(); }
double KnToMph(double value) { return MphT(FpsT(KnT(value))).Value(); }

// Time
double MsToS(double value) { return SecT(MsecT(value)).Value(); }
double UsToS(double value) { return SecT(UsecT(value)).Value(); }
double SToMs(double value) { return MsecT(SecT(value)).Value(); }
double SToUs(double value) { return UsecT(SecT(value)).Value(); }

// Temperature
double DegCToDegF(double value) { return DegFT(DegCT(value)).Value(); }
}  // namespace lob

// This file is part of lob.
//
// lob is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// lob is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
