// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <cassert>
#include <cmath>

#include "eng_units.hpp"
#include "helpers.hpp"
#include "lob/lob.h"
#include "version.hpp"

namespace lob {
namespace {

constexpr double kHundredYardsInFeet = FeetT(YardT(100)).Value();

}  // namespace

extern "C" {

const char* LobVersion(void) { return kProjectVersion; }

double LobMoaToMil(double value) { return MilT(MoaT(value)).Value(); }
double LobMoaToDeg(double value) { return DegreesT(MoaT(value)).Value(); }
double LobMoaToIphy(double value) { return IphyT(MoaT(value)).Value(); }
double LobMoaToInch(double value, double range_ft) {
  return IphyT(MoaT(value)).Value() * range_ft / kHundredYardsInFeet;
}

double LobMilToMoa(double value) { return MoaT(MilT(value)).Value(); }
double LobMilToDeg(double value) { return DegreesT(MilT(value)).Value(); }
double LobMilToIphy(double value) { return IphyT(MilT(value)).Value(); }
double LobMilToInch(double value, double range_ft) {
  return IphyT(MilT(value)).Value() * range_ft / kHundredYardsInFeet;
}

double LobDegToMoa(double value) { return MoaT(DegreesT(value)).Value(); }
double LobDegToMil(double value) { return MilT(DegreesT(value)).Value(); }

double LobInchToMoa(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return MoaT(IphyT(value / (range_ft / kHundredYardsInFeet))).Value();
}

double LobInchToMil(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return MilT(IphyT(value / (range_ft / kHundredYardsInFeet))).Value();
}

double LobInchToDeg(double value, double range_ft) {
  if (AreEqual(range_ft, 0.0)) {
    return 0;
  }
  return DegreesT(IphyT(value / (range_ft / kHundredYardsInFeet))).Value();
}

double LobJToFtLbs(double value) { return FtLbsT(JouleT(value)).Value(); }
double LobFtLbsToJ(double value) { return JouleT(FtLbsT(value)).Value(); }

double LobMToYd(double value) { return YardT(MeterT(value)).Value(); }
double LobYdToFt(double value) { return FeetT(YardT(value)).Value(); }
double LobMToFt(double value) { return FeetT(MeterT(value)).Value(); }
double LobFtToIn(double value) { return InchT(FeetT(value)).Value(); }
double LobMmToIn(double value) { return InchT(MmT(value)).Value(); }
double LobCmToIn(double value) { return InchT(CmT(value)).Value(); }
double LobYdToM(double value) { return MeterT(YardT(value)).Value(); }
double LobFtToM(double value) { return MeterT(FeetT(value)).Value(); }
double LobFtToYd(double value) { return YardT(FeetT(value)).Value(); }
double LobInToMm(double value) { return MmT(InchT(value)).Value(); }
double LobInToCm(double value) { return CmT(InchT(value)).Value(); }
double LobInToFt(double value) { return FeetT(InchT(value)).Value(); }

double LobPaToInHg(double value) { return InHgT(PaT(value)).Value(); }
double LobMbarToInHg(double value) { return InHgT(MbarT(value)).Value(); }
double LobPsiToInHg(double value) { return InHgT(PsiT(value)).Value(); }

double LobLbsToGrain(double value) { return GrainT(LbsT(value)).Value(); }
double LobGToGrain(double value) { return GrainT(LbsT(GramT(value))).Value(); }
double LobKgToGrain(double value) { return GrainT(LbsT(KgT(value))).Value(); }

double LobKgSqMToPmsi(double value) { return PmsiT(KgsmT(value)).Value(); }

double LobFpsToMps(double value) { return MpsT(FpsT(value)).Value(); }
double LobMpsToFps(double value) { return FpsT(MpsT(value)).Value(); }
double LobKphToMph(double value) { return MphT(FpsT(KphT(value))).Value(); }
double LobKnToMph(double value) { return MphT(FpsT(KnT(value))).Value(); }

double LobMsToS(double value) { return SecT(MsecT(value)).Value(); }
double LobUsToS(double value) { return SecT(UsecT(value)).Value(); }
double LobSToMs(double value) { return MsecT(SecT(value)).Value(); }
double LobSToUs(double value) { return UsecT(SecT(value)).Value(); }

double LobDegCToDegF(double value) { return DegFT(DegCT(value)).Value(); }

}  // extern "C"
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
