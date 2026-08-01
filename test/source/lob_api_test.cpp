// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

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
  const lob::Context kA{};
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
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .MaximumTime(kMaxTime)
                                   .Build();

  const uint32_t kRange = 5'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
  EXPECT_NEAR(out.time_of_flight, kMaxTime, 1E-3);
  EXPECT_GT(out.time_of_flight, 0.0);
  EXPECT_GT(out.velocity, 0);
}

TEST(LobAPITest, MinimumVelocity) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const uint16_t kMinimumVelocity = 2'000U;
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .MinimumSpeed(kMinimumVelocity)
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
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .MassGrains(kGrains)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .MinimumEnergy(kMinimumEnergy)
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
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .MassGrains(kGrains)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  const uint32_t kRange = 50'000U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, &kRange, &out, 1U);
  EXPECT_EQ(kSize, 1);
  EXPECT_LT(out.range, kRange);
}

TEST(LobAPITest, FreeFallTermination) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
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
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  const std::array<uint32_t, 2> kRanges = {100U, 50U};
  std::array<lob::Output, 2> out{};
  const auto kSize = lob::Solve(kResult, kRanges, &out);
  EXPECT_EQ(kSize, 0);
}

TEST(LobAPITest, ZeroAngleSearchNegativeBracket) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(0.436)
                                .InitialVelocityFps(3100U)
                                .ZeroDistanceYds(100.0)
                                .ZeroImpactHeightInches(-60.0)
                                .OpticHeightInches(3.0)
                                .Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kNone);
  EXPECT_FALSE(std::isnan(kCtx.zero_angle));
  EXPECT_LT(kCtx.zero_angle, 0.0);
  EXPECT_GT(kCtx.zero_angle, -45.0 * 60.0);
}

TEST(LobAPITest, ZeroAngleSearchExhaustsIterations) {
  const lob::Context kCtx = lob::Builder()
                                .BallisticCoefficientPsi(0.1)
                                .InitialVelocityFps(600U)
                                .ZeroDistanceYds(2000.0)
                                .Build();
  EXPECT_EQ(kCtx.error, lob::ErrorT::kInternalError);
  EXPECT_TRUE(std::isnan(kCtx.zero_angle));
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
TEST(LobAPITest, ZeroAngleSearchExtremeInputsDontCrash) {
  const std::array<double, 5> kBcs = {0.1, 0.224, 0.436, 0.7, 1.5};
  const std::array<uint16_t, 5> kVelocities = {600U, 1800U, 3100U, 4000U,
                                               5000U};
  const std::array<double, 5> kRanges = {10.0, 100.0, 500.0, 1000.0, 2000.0};
  for (const double kBc : kBcs) {
    for (const uint16_t kV : kVelocities) {
      for (const double kRange : kRanges) {
        const lob::Context kCtx = lob::Builder()
                                      .BallisticCoefficientPsi(kBc)
                                      .InitialVelocityFps(kV)
                                      .ZeroDistanceYds(kRange)
                                      .Build();
        const bool kOk = (kCtx.error == lob::ErrorT::kNone);
        const bool kFailed = (kCtx.error == lob::ErrorT::kInternalError);
        EXPECT_TRUE(kOk || kFailed)
            << "BC=" << kBc << " v=" << kV << " d=" << kRange
            << " error=" << static_cast<int>(kCtx.error);
        if (kOk) {
          EXPECT_FALSE(std::isnan(kCtx.zero_angle))
              << "BC=" << kBc << " v=" << kV << " d=" << kRange;
          EXPECT_GE(kCtx.zero_angle, -45.0 * 60.0)
              << "BC=" << kBc << " v=" << kV << " d=" << kRange;
          EXPECT_LE(kCtx.zero_angle, 45.0 * 60.0)
              << "BC=" << kBc << " v=" << kV << " d=" << kRange;
        }
      }
    }
  }
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
  constexpr std::array<std::pair<lob::ErrorT, ::LobErrorT>, 31> kErrors = {
      {{lob::ErrorT::kNone, ::kLobErrorNone},
       {lob::ErrorT::kAirPressureOOR, ::kLobErrorAirPressureOOR},
       {lob::ErrorT::kAltitudeOfBarometerOOR,
        ::kLobErrorAltitudeOfBarometerOOR},
       {lob::ErrorT::kAltitudeOfFiringSiteOOR,
        ::kLobErrorAltitudeOfFiringSiteOOR},
       {lob::ErrorT::kAltitudeOfThermometerOOR,
        ::kLobErrorAltitudeOfThermometerOOR},
       {lob::ErrorT::kAzimuthOOR, ::kLobErrorAzimuthOOR},
       {lob::ErrorT::kBallisticCoefficientOOR,
        ::kLobErrorBallisticCoefficientOOR},
       {lob::ErrorT::kBallisticCoefficientRequired,
        ::kLobErrorBallisticCoefficientRequired},
       {lob::ErrorT::kBaseDiameterOOR, ::kLobErrorBaseDiameterOOR},
       {lob::ErrorT::kDiameterOOR, ::kLobErrorDiameterOOR},
       {lob::ErrorT::kHumidityOOR, ::kLobErrorHumidityOOR},
       {lob::ErrorT::kInitialVelocityRequired,
        ::kLobErrorInitialVelocityRequired},
       {lob::ErrorT::kInternalError, ::kLobErrorInternalError},
       {lob::ErrorT::kLatitudeOOR, ::kLobErrorLatitudeOOR},
       {lob::ErrorT::kLengthOOR, ::kLobErrorLengthOOR},
       {lob::ErrorT::kMachDragTableInvalid, ::kLobErrorMachDragTableInvalid},
       {lob::ErrorT::kMachDragTableNotMonotonic,
        ::kLobErrorMachDragTableNotMonotonic},
       {lob::ErrorT::kMachDragTableTooNarrow,
        ::kLobErrorMachDragTableTooNarrow},
       {lob::ErrorT::kMachDragTableTooShort, ::kLobErrorMachDragTableTooShort},
       {lob::ErrorT::kMassOOR, ::kLobErrorMassOOR},
       {lob::ErrorT::kMaximumTimeOOR, ::kLobErrorMaximumTimeOOR},
       {lob::ErrorT::kMeplatDiameterOOR, ::kLobErrorMeplatDiameterOOR},
       {lob::ErrorT::kNoseLengthOOR, ::kLobErrorNoseLengthOOR},
       {lob::ErrorT::kOgiveRtROOR, ::kLobErrorOgiveRtROOR},
       {lob::ErrorT::kRangeAngleOOR, ::kLobErrorRangeAngleOOR},
       {lob::ErrorT::kTailLengthOOR, ::kLobErrorTailLengthOOR},
       {lob::ErrorT::kWindHeadingOOR, ::kLobErrorWindHeadingOOR},
       {lob::ErrorT::kZeroAngleOOR, ::kLobErrorZeroAngleOOR},
       {lob::ErrorT::kZeroDataRequired, ::kLobErrorZeroDataRequired},
       {lob::ErrorT::kZeroDistanceOOR, ::kLobErrorZeroDistanceOOR},
       {lob::ErrorT::kNotFormed, ::kLobErrorNotFormed}}};
  static_assert(kErrors.size() == static_cast<size_t>(::kLobErrorNotFormed) + 1,
                "Error enumeration changed; update the pair table above");
  for (const auto& error : kErrors) {
    EXPECT_TRUE(error.first == error.second)
        << "error=" << static_cast<int>(error.first);
    EXPECT_TRUE(error.second == error.first)
        << "error=" << static_cast<int>(error.first);
    EXPECT_FALSE(error.first != error.second)
        << "error=" << static_cast<int>(error.first);
    EXPECT_FALSE(error.second != error.first)
        << "error=" << static_cast<int>(error.first);
  }
}

TEST(LobAPITest, CEnumBCAtmosphereOverload) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .BCAtmosphere(1)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kNone);
}

TEST(LobAPITest, CEnumBCDragFunctionOverload) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .BCDragFunction(1)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kNone);
}

TEST(LobAPITest, CEnumWindHeadingOverload) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .WindHeading(12)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kNone);
}

TEST(LobAPITest, SingleRangeSolveOverload) {
  const double kTestBC = 0.436;
  const uint16_t kTestMuzzleVelocity = 3100U;
  const double kTestZeroAngle = 6.11;
  const lob::Context kResult = lob::Builder()
                                   .BallisticCoefficientPsi(kTestBC)
                                   .InitialVelocityFps(kTestMuzzleVelocity)
                                   .ZeroAngleMOA(kTestZeroAngle)
                                   .Build();
  EXPECT_EQ(kResult.error, lob::ErrorT::kNone);
  const uint32_t kRange = 30U;
  lob::Output out;
  const auto kSize = lob::Solve(kResult, kRange, &out);
  EXPECT_EQ(kSize, 1);
  EXPECT_TRUE(std::isfinite(out.range));
  EXPECT_TRUE(std::isfinite(out.elevation));
  EXPECT_TRUE(std::isfinite(out.time_of_flight));
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