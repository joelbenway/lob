// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

import {
  Atmosphere,
  Builder,
  Context,
  DragFunction,
  ForwardTrajectoryRow,
  InverseTrajectoryRow,
  LobError,
  WindPoint,
  degCToDegF,
  moaToInch,
  version,
} from "@joelbenway/lob";
import createLobModule, { type LobRawModule } from "@joelbenway/lob/raw";

// 1. Valid Builder chaining and setters
const builder: Builder = new Builder()
  .ballisticCoefficientPsi(0.243)
  .bcAtmosphere(Atmosphere.ICAO)
  .bcAtmosphere("ICAO")
  .bcAtmosphere("ArmyStandardMetro")
  .bcDragFunction(DragFunction.G7)
  .bcDragFunction("G7")
  .bcDragFunction("G1")
  .diameterInch(0.308)
  .meplatDiameterInch(0.06)
  .baseDiameterInch(0.25)
  .lengthInch(1.2)
  .noseLengthInch(0.6)
  .tailLengthInch(0.15)
  .ogiveRtR(0.5)
  .massGrains(168)
  .initialVelocityFps(2700)
  .opticHeightInches(1.5)
  .twistInchesPerTurn(11)
  .zeroAngleMoa(0)
  .zeroDistanceYds(100)
  .zeroImpactHeightInches(0)
  .altitudeOfFiringSiteFt(1000)
  .airPressureInHg(29.92)
  .altitudeOfBarometerFt(1000)
  .temperatureDegF(59)
  .altitudeOfThermometerFt(1000)
  .relativeHumidityPercent(50)
  .windHeading(3)
  .windHeadingDeg(90)
  .windSpeedFps(14.67)
  .windSpeedMph(10)
  .windShearExponent(0.14)
  .azimuthDeg(0)
  .latitudeDeg(45)
  .rangeAngleDeg(0)
  .minimumSpeed(1000)
  .minimumEnergy(500)
  .maximumTime(5.0)
  .stepSize(1000);

// Array / table inputs
builder.machVsDragTable([1.5, 2.0], [0.2, 0.3]);
builder.machVsDragTable(
  new Float32Array([1.5, 2.0]),
  new Float32Array([0.2, 0.3]),
);
builder.bcVelocityBands([2000, 3000], [0.25, 0.25]);
builder.splineCoefficients(new Float32Array(60).fill(0.1));

// Wind profile
const points: readonly WindPoint[] = [
  { rangeFt: 0, headingDeg: 90, speedMph: 10 },
  { rangeFt: 1000, headingDeg: 90, speedMph: 10, heightFt: 10 },
];
builder.windProfile(points);

// Build Context
const context: Context = builder.build();

// 2. Forward solutions
const forwardFt: ForwardTrajectoryRow[] = context.solveFt([0, 300, 600]);
const forwardYds: ForwardTrajectoryRow[] = context.solveYds([0, 100, 200]);
const forwardELRFt: ForwardTrajectoryRow[] = context.solveELRFt([0, 300, 600]);
const forwardELRYds: ForwardTrajectoryRow[] = context.solveELRYds([
  0, 100, 200,
]);

const fRow: ForwardTrajectoryRow = forwardYds[0];
const rFt: number = fRow.rangeFt;
const rYds: number = fRow.rangeYds;
const vFps: number = fRow.velocityFps;
const eFtLbs: number = fRow.energyFtLbs;
const elInch: number = fRow.elevationInch;
const defInch: number = fRow.deflectionInch;
const tofSec: number = fRow.timeOfFlightSec;

// 3. Inverse solutions
const inverseFt: InverseTrajectoryRow[] = context.solveInverseFt([100, 200]);
const inverseYds: InverseTrajectoryRow[] = context.solveInverseYds([100, 200]);
const inverseELRFt: InverseTrajectoryRow[] = context.solveInverseELRFt([
  100, 200,
]);
const inverseELRYds: InverseTrajectoryRow[] = context.solveInverseELRYds([
  100, 200,
]);

const iRow: InverseTrajectoryRow = inverseYds[0];
const elMoa: number = iRow.elevationMoa;
const defMoa: number = iRow.deflectionMoa;

// 4. Unit conversions and version
const ver: string = version();
const inVal: number = moaToInch(1.0, 300);
const degF: number = degCToDegF(20);

// 5. Error handling
try {
  new Builder().initialVelocityFps(0).build();
} catch (error) {
  if (error instanceof LobError) {
    const code: number = error.code;
  }
}

// 6. Raw module entry point
async function checkRaw(): Promise<void> {
  const raw: LobRawModule = await createLobModule();
  const vPtr: number = raw._LobVersion();
  const vString: string = raw.UTF8ToString(vPtr);
  const mem: number = raw._malloc(64);
  raw._free(mem);
  const u8: Uint8Array = raw.HEAPU8;
}

// 7. Lifecycle methods
context.dispose();
builder.reset();
builder.dispose();

// 8. Expected compile-time failures for invalid calls

// @ts-expect-error - forward row must not have elevationMoa
const badMoa: number = fRow.elevationMoa;

// @ts-expect-error - forward row must not have deflectionMoa
const badDefMoa: number = fRow.deflectionMoa;

// @ts-expect-error - inverse row must not have elevationInch
const badInch: number = iRow.elevationInch;

// @ts-expect-error - inverse row must not have deflectionInch
const badDefInch: number = iRow.deflectionInch;

// @ts-expect-error - invalid drag function string
builder.bcDragFunction("G99");

// @ts-expect-error - invalid atmosphere string
builder.bcAtmosphere("InvalidAtmosphere");

// @ts-expect-error - scalar setter rejects string
builder.diameterInch("0.308");

// @ts-expect-error - Context constructor is private
const directContext = new Context();

// @ts-expect-error - windProfile requires objects with rangeFt, headingDeg, speedMph
builder.windProfile([{ rangeFt: 0 }]);

export {
  elInch,
  elMoa,
  ver,
  inVal,
  degF,
  rFt,
  rYds,
  vFps,
  eFtLbs,
  defInch,
  tofSec,
  defMoa,
};

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
