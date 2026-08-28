// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cstdint>

namespace lob {
constexpr double kPi = 3.14159265358979323846;
constexpr int32_t kDegreesPerTurn = 360;
constexpr double kStandardGravityFtPerSecSq = 32.17405;

namespace isa {
constexpr double kSeaLevelDegF = 59.0;
constexpr double kSeaLevelHumidityPercent = 0.0;
constexpr double kSeaLevelPressureInHg = 29.92;
constexpr double kSeaLevelAirDensityLbsPerCuFt = 0.0764742;
constexpr double kSeaLevelSpeedOfSoundFps = 1116.45;
constexpr double kLapseDegFPerFt = 0.00356616;
constexpr double kTropopauseAltitudeFt = 36'090.0;
constexpr double kStratosphereAltitudeFt = 65'617.0;
constexpr double kMinimumTempDegF = -69.7;
constexpr double kGasConstantAir = 1716.46;  // ft·lbf / (slug·°R)

constexpr double kHydrostaticExponent =
    (kStandardGravityFtPerSecSq / (kGasConstantAir * kLapseDegFPerFt)) - 1.0;
constexpr double kBarometricExponent = kHydrostaticExponent + 1.0;
}  // namespace isa

constexpr double kArmySeaLevelAirDensityLbsPerCuFt = 0.0751265;
constexpr double kArmyToIcaoBcConversionFactor =
    kArmySeaLevelAirDensityLbsPerCuFt / isa::kSeaLevelAirDensityLbsPerCuFt;
constexpr double kAngularVelocityOfEarthRadPerSec = 7.292115E-5;
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
