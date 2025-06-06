// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <cstdint>

namespace lob {
constexpr double kPi = 3.14159265358979323846;
constexpr int32_t kDegreesPerTurn = 360;
constexpr double kStandardGravityFtPerSecSq = 32.1740;
constexpr double kIsaSeaLevelDegF = 59.0;
constexpr double kIsaSeaLevelHumidityPercent = 0.0;
constexpr double kIsaSeaLevelPressureInHg = 29.92;
constexpr double kIsaSeaLevelAirDensityLbsPerCuFt = 0.0764742;
constexpr double kIsaSeaLevelSpeedOfSoundFps = 1116.45;
constexpr double kIsaLapseDegFPerFt = 0.00356616;
constexpr double kIsaTropopauseAltitudeFt = 36'090.0;
constexpr double kIsaMinimumTempDegF = -69.7;
constexpr double kArmyToIcaoBcConversionFactor = 0.982;
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