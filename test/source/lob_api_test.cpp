// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

#include "eng_units.hpp"
#include "lob/lob.h"
#include "lob/lob.hpp"

namespace tests {

TEST(LobAPITest, Version) {
  const char* version_string = lob::Version();
  EXPECT_NE(version_string, nullptr);
  uint8_t dot_count = 0U;
  for (uint8_t i = 0U; version_string[i] != '\0'; i++) {
    if (version_string[i] == '.') {
      dot_count++;
    }
  }
  EXPECT_EQ(2, dot_count) << version_string;
}

TEST(LobAPITest, SolverSkipsPoorlyFormedInput) {
  const lob::Input kA{};
  const uint32_t kB = 100U;
  lob::Output out;
  const auto kSize = lob::Solve(kA, &kB, &out, 1U);
  EXPECT_EQ(kSize, 0);
}

TEST(LobAPITest, MaximumTimeOfFlight) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const double kMaxTime = 1.5;
  const lob::Input kResult = lob::Builder()
                                 .BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .MaximumTime(kMaxTime)
                                 .StepSize(100U)
                                 .Build();

  const uint32_t kRange = 5'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
  EXPECT_NEAR(out.time_of_flight, kMaxTime, 1E-3);
}

TEST(LobAPITest, MinimumVelocity) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const uint16_t kMinimumVelocity = 2'000U;
  const lob::Input kResult = lob::Builder()
                                 .BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .MinimumSpeed(kMinimumVelocity)
                                 .StepSize(100U)
                                 .Build();
  const uint32_t kRange = 5'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
  EXPECT_EQ(out.velocity, kMinimumVelocity);
}

TEST(LobAPITest, MinimumEnergy) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kGrains = 130.0;
  const double kTestZeroAngle = 6.11;
  const uint16_t kMinimumEnergy = 1'000U;
  const lob::Input kResult = lob::Builder()
                                 .BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .MassGrains(kGrains)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .MinimumEnergy(kMinimumEnergy)
                                 .StepSize(100U)
                                 .Build();
  const uint32_t kRange = 5'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
  EXPECT_EQ(out.energy, kMinimumEnergy);
}

TEST(LobAPITest, RunUntilFallStop) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kGrains = 130.0;
  const double kTestZeroAngle = 6.11;
  const lob::Input kResult = lob::Builder()
                                 .BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .MassGrains(kGrains)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .StepSize(10'000U)
                                 .Build();
  const uint32_t kRange = 50'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
  EXPECT_LT(out.range, kRange);
}

TEST(LobAPITest, ZeroStepSizeAutoTerminate) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Input kResult = lob::Builder()
                                 .BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .StepSize(0U)
                                 .Build();
  const uint32_t kRange = 50'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
}

TEST(LobAPITest, NonMonotonicRangesRejected) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Input kResult = lob::Builder()
                                 .BallisticCoefficientPsi(kTestBC)
                                 .InitialVelocityFps(kTestMuzzleVelocity)
                                 .ZeroAngleMOA(kTestZeroAngle)
                                 .Build();
  const std::array<uint32_t, 2> kRanges = {100U, 50U};
  std::array<lob::Output, 2> out{};
  const auto kSize = lob::Solve(kResult, kRanges, out);
  EXPECT_EQ(kSize, 0);
}

TEST(LobAPITest, MoaToMil) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::MilT(lob::MoaToMil(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToDeg) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::DegreesT(lob::MoaToDeg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToIphy) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::IphyT(lob::MoaToIphy(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MoaT(kB).Value());
}

TEST(LobAPITest, MoaToInch) {
  const auto kA = lob::MoaT(5);
  const auto kB = lob::MoaToInch(kA.Value(), 300);
  EXPECT_DOUBLE_EQ(lob::IphyT(kA).Value(), kB);
}

TEST(LobAPITest, MilToMoa) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::MoaT(lob::MilToMoa(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MilT(kB).Value());
}

TEST(LobAPITest, MilToDeg) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::DegreesT(lob::MilToDeg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MilT(kB).Value());
}

TEST(LobAPITest, MilToIphy) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::IphyT(lob::MilToIphy(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MilT(kB).Value());
}

TEST(LobAPITest, MilToInch) {
  const auto kA = lob::MilT(10);
  const auto kB = lob::MilToInch(kA.Value(), 300);
  EXPECT_DOUBLE_EQ(lob::IphyT(kA).Value(), kB);
}

TEST(LobAPITest, DegToMoa) {
  const auto kA = lob::DegreesT(0.2);
  const auto kB = lob::MoaT(lob::DegToMoa(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::DegreesT(kB).Value());
}

TEST(LobAPITest, DegToMil) {
  const auto kA = lob::DegreesT(0.2);
  const auto kB = lob::MilT(lob::DegToMil(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::DegreesT(kB).Value());
}

TEST(LobAPITest, InchToMoa) {
  const auto kA = lob::IphyT(5);
  const auto kB = lob::MoaT(lob::InchToMoa(kA.Value(), 300));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::IphyT(kB).Value());
  ;
  EXPECT_DOUBLE_EQ(lob::InchToMoa(kA.Value(), 0.0), 0.0);
}

TEST(LobAPITest, InchToMil) {
  const auto kA = lob::IphyT(5);
  const auto kB = lob::MilT(lob::InchToMil(kA.Value(), 300));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::IphyT(kB).Value());
  EXPECT_DOUBLE_EQ(lob::InchToMil(kA.Value(), 0.0), 0.0);
}

TEST(LobAPITest, InchToDeg) {
  const auto kA = lob::IphyT(5);
  const auto kB = lob::DegreesT(lob::InchToDeg(kA.Value(), 300));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::IphyT(kB).Value());
}

TEST(LobAPITest, InchToDegZero) {
  const auto kA = lob::IphyT(5);
  const auto kB = lob::DegreesT(lob::InchToDeg(kA.Value(), 0.0));
  EXPECT_DOUBLE_EQ(0.0, lob::IphyT(kB).Value());
}

TEST(LobAPITest, JToFtLbs) {
  const auto kA = lob::JouleT(100);
  const auto kB = lob::FtLbsT(lob::JToFtLbs(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::JouleT(kB).Value());
}

TEST(LobAPITest, FtLbsToJ) {
  const auto kA = lob::FtLbsT(100);
  const auto kB = lob::JouleT(lob::FtLbsToJ(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FtLbsT(kB).Value());
}

TEST(LobAPITest, MToYd) {
  const auto kA = lob::MeterT(100);
  const auto kB = lob::YardT(lob::MToYd(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MeterT(kB).Value());
}

TEST(LobAPITest, YdToFt) {
  const auto kA = lob::YardT(100);
  const auto kB = lob::FeetT(lob::YdToFt(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::YardT(kB).Value());
}

TEST(LobAPITest, MToFt) {
  const auto kA = lob::MeterT(100);
  const auto kB = lob::FeetT(lob::MToFt(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MeterT(kB).Value());
}

TEST(LobAPITest, FtToIn) {
  const auto kA = lob::FeetT(100);
  const auto kB = lob::InchT(lob::FtToIn(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FeetT(kB).Value());
}

TEST(LobAPITest, MmToIn) {
  const auto kA = lob::MmT(100);
  const auto kB = lob::InchT(lob::MmToIn(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MmT(kB).Value());
}

TEST(LobAPITest, CmToIn) {
  const auto kA = lob::CmT(100);
  const auto kB = lob::InchT(lob::CmToIn(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::CmT(kB).Value());
}

TEST(LobAPITest, YdToM) {
  const auto kA = lob::YardT(100);
  const auto kB = lob::MeterT(lob::YdToM(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::YardT(kB).Value());
}

TEST(LobAPITest, FtToM) {
  const auto kA = lob::FeetT(100);
  const auto kB = lob::MeterT(lob::FtToM(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FeetT(kB).Value());
}

TEST(LobAPITest, FtToYd) {
  const auto kA = lob::FeetT(100);
  const auto kB = lob::YardT(lob::FtToYd(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FeetT(kB).Value());
}

TEST(LobAPITest, InToMm) {
  const auto kA = lob::InchT(100);
  const auto kB = lob::MmT(lob::InToMm(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::InchT(kB).Value());
}

TEST(LobAPITest, InToCm) {
  const auto kA = lob::InchT(100);
  const auto kB = lob::CmT(lob::InToCm(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::InchT(kB).Value());
}

TEST(LobAPITest, InToFt) {
  const auto kA = lob::InchT(100);
  const auto kB = lob::FeetT(lob::InToFt(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::InchT(kB).Value());
}

TEST(LobAPITest, PaToInHg) {
  const auto kA = lob::PaT(100);
  const auto kB = lob::InHgT(lob::PaToInHg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::PaT(kB).Value());
}

TEST(LobAPITest, MbarToInHg) {
  const auto kA = lob::MbarT(100);
  const auto kB = lob::InHgT(lob::MbarToInHg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MbarT(kB).Value());
}

TEST(LobAPITest, PsiToInHg) {
  const auto kA = lob::PsiT(100);
  const auto kB = lob::InHgT(lob::PsiToInHg(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::PsiT(kB).Value());
}

TEST(LobAPITest, LbsToGrain) {
  const auto kA = lob::LbsT(100);
  const auto kB = lob::GrainT(lob::LbsToGrain(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::LbsT(kB).Value());
}

TEST(LobAPITest, GToGrain) {
  const auto kA = lob::GramT(100);
  const auto kB = lob::GrainT(lob::GToGrain(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::GramT(kB).Value());
}

TEST(LobAPITest, KgToGrain) {
  const auto kA = lob::KgT(100);
  const auto kB = lob::GrainT(lob::KgToGrain(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::KgT(kB).Value());
}

TEST(LobAPITest, KgSqMToPmsi) {
  const auto kA = lob::KgsmT(100);
  const auto kB = lob::PmsiT(lob::KgSqMToPmsi(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::KgsmT(kB).Value());
}

TEST(LobAPITest, FpsToMps) {
  const auto kA = lob::FpsT(100);
  const auto kB = lob::MpsT(lob::FpsToMps(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::FpsT(kB).Value());
}

TEST(LobAPITest, MpsToFps) {
  const auto kA = lob::MpsT(100);
  const auto kB = lob::FpsT(lob::MpsToFps(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MpsT(kB).Value());
}

TEST(LobAPITest, KphToMph) {
  const auto kA = lob::KphT(100);
  const auto kB = lob::MphT(lob::KphToMph(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::KphT(kB).Value());
}

TEST(LobAPITest, KnToMph) {
  const auto kA = lob::KnT(100);
  const auto kB = lob::MphT(lob::KnToMph(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::KnT(kB).Value());
}

TEST(LobAPITest, MsToS) {
  const auto kA = lob::MsecT(100);
  const auto kB = lob::SecT(lob::MsToS(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::MsecT(kB).Value());
}

TEST(LobAPITest, UsToS) {
  const auto kA = lob::UsecT(100);
  const auto kB = lob::SecT(lob::UsToS(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::UsecT(kB).Value());
}

TEST(LobAPITest, SToMs) {
  const auto kA = lob::SecT(100);
  const auto kB = lob::MsecT(lob::SToMs(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::SecT(kB).Value());
}

TEST(LobAPITest, SToUs) {
  const auto kA = lob::SecT(100);
  const auto kB = lob::UsecT(lob::SToUs(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::SecT(kB).Value());
}

TEST(LobAPITest, DegCToDegF) {
  const auto kA = lob::DegCT(100);
  const auto kB = lob::DegFT(lob::DegCToDegF(kA.Value()));
  EXPECT_DOUBLE_EQ(kA.Value(), lob::DegCT(kB).Value());
}

TEST(LobAPITest, ErrorTComparisonWithCEnum) {
  EXPECT_TRUE(lob::ErrorT::kNone == kLobErrorNone);
  EXPECT_TRUE(kLobErrorNone == lob::ErrorT::kNone);
  EXPECT_FALSE(lob::ErrorT::kNone == kLobErrorBallisticCoefficientRequired);
  EXPECT_FALSE(kLobErrorBallisticCoefficientRequired == lob::ErrorT::kNone);
  EXPECT_TRUE(lob::ErrorT::kNone != kLobErrorBallisticCoefficientRequired);
  EXPECT_TRUE(kLobErrorBallisticCoefficientRequired != lob::ErrorT::kNone);
  EXPECT_FALSE(lob::ErrorT::kNone != kLobErrorNone);
  EXPECT_FALSE(kLobErrorNone != lob::ErrorT::kNone);
}

TEST(LobAPITest, ToEnumDragFunction) {
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(kLobDragFunctionG1),
            lob::DragFunctionT::kG1);
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(kLobDragFunctionG2),
            lob::DragFunctionT::kG2);
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(kLobDragFunctionG5),
            lob::DragFunctionT::kG5);
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(kLobDragFunctionG6),
            lob::DragFunctionT::kG6);
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(kLobDragFunctionG7),
            lob::DragFunctionT::kG7);
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(kLobDragFunctionG8),
            lob::DragFunctionT::kG8);
}

TEST(LobAPITest, ToEnumAtmosphereReference) {
  EXPECT_EQ(lob::ToEnum<lob::AtmosphereReferenceT>(
                kLobAtmosphereReferenceArmyStandardMetro),
            lob::AtmosphereReferenceT::kArmyStandardMetro);
  EXPECT_EQ(lob::ToEnum<lob::AtmosphereReferenceT>(kLobAtmosphereReferenceIcao),
            lob::AtmosphereReferenceT::kIcao);
}

TEST(LobAPITest, ToUnderlyingClockAngle) {
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kI), kLobClockAngleI);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kII), kLobClockAngleII);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kIII), kLobClockAngleIII);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kIV), kLobClockAngleIV);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kV), kLobClockAngleV);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kVI), kLobClockAngleVI);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kVII), kLobClockAngleVII);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kVIII), kLobClockAngleVIII);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kIX), kLobClockAngleIX);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kX), kLobClockAngleX);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kXI), kLobClockAngleXI);
  EXPECT_EQ(lob::ToUnderlying(lob::ClockAngleT::kXII), kLobClockAngleXII);
}

TEST(LobAPITest, ToUnderlyingError) {
  EXPECT_EQ(lob::ToUnderlying(lob::ErrorT::kHumidityOOR), kLobErrorHumidityOOR);
}

TEST(LobAPITest, ToEnumToUnderlyingRoundTrip) {
  EXPECT_EQ(lob::ToEnum<lob::DragFunctionT>(
                lob::ToUnderlying(lob::DragFunctionT::kG8)),
            lob::DragFunctionT::kG8);
  EXPECT_EQ(
      lob::ToEnum<lob::ClockAngleT>(lob::ToUnderlying(lob::ClockAngleT::kXII)),
      lob::ClockAngleT::kXII);
  EXPECT_EQ(lob::ToEnum<lob::AtmosphereReferenceT>(lob::ToUnderlying(
                lob::AtmosphereReferenceT::kArmyStandardMetro)),
            lob::AtmosphereReferenceT::kArmyStandardMetro);
  EXPECT_EQ(
      lob::ToEnum<lob::ErrorT>(lob::ToUnderlying(lob::ErrorT::kInternalError)),
      lob::ErrorT::kInternalError);
}

}  // namespace tests

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