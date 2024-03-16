// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include <iostream>

#include "lob/lob.hpp"

namespace {
const double kExampleBC = 0.425;
const double kExampleDiameter = 0.308;
const double kExampleWeight = 180.0;
const double kExampleMuzzleVelocity = 3000.0;
const double kExampleZero = 100.0;
const double kExampleOpticHeight = 1.5;
const double kTargetDistance = 1000;
const double kExamplePressure = 29.53;
const double kExampleTemp = 59;
const double kExampleHumidity = 78;
const double kExampleAltitude = 10'000;
const double kExampleNewAltitude = 11'000;
}  // namespace

int main() {
  auto shot = lob::Lob::Builder()
                  .BallisticCoefficentPsi(kExampleBC)
                  .BCDragFunction(lob::DragFunctionT::kG1)
                  .BCAtmosphere(lob::AtmosphereReferenceT::kIcao)
                  .DiameterInch(kExampleDiameter)
                  .MassGrains(kExampleWeight)
                  .InitialVelocityFps(kExampleMuzzleVelocity)
                  .ZeroDistanceYds(kExampleZero)
                  .OpticHeightInches(kExampleOpticHeight)
                  .TargetDistanceYds(kTargetDistance)
                  .BarometricPressureInHg(kExamplePressure)
                  .TemperatureDegF(kExampleTemp)
                  .RelativeHumidityPercent(kExampleHumidity)
                  .AltitudeFt(kExampleAltitude)
                  .Build();

  shot = lob::Lob::Builder(*shot).AltitudeFt(kExampleNewAltitude).Build();

  std::cout << "complete\n";

  return 0;
}

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