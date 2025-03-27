// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "eng_units.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>

namespace tests {

TEST(EngUnitsTests, StrongTConstructors) {
  enum class Beer : uint8_t { kBottle, kCan, kSixPack, kCase };
  using BottleT = lob::StrongT<Beer, Beer::kBottle, int>;
  const BottleT kBreakfast(1);
  const BottleT kLunch(kBreakfast);
  const BottleT kDinner = BottleT(1);
  const double kSum = static_cast<int>(kBreakfast + kLunch + kDinner);
  EXPECT_DOUBLE_EQ(kSum, 3.0);
}

TEST(EngUnitsTests, StrongTArithmeticAdd) {
  enum class Waffles : uint8_t { kBelgian, kToaster, kPotato, kStroop };
  using BelgianT = lob::StrongT<Waffles, Waffles::kBelgian, int>;
  const int kTest1 = 77;
  const int kTest2 = 600;
  constexpr auto kExpectedSum = kTest1 + kTest2;
  const BelgianT kPlate1(kTest1);
  const BelgianT kPlate2(kTest2);
  EXPECT_EQ((kPlate1 + kPlate2).Value(), kExpectedSum);
  EXPECT_EQ((kPlate1 + kTest2).Value(), kExpectedSum);
  BelgianT plate3 = kPlate1;
  plate3 += kPlate2;
  EXPECT_EQ(plate3.Value(), kExpectedSum);
  plate3 = kPlate1;
  plate3 += kTest2;
  EXPECT_EQ(plate3.Value(), kExpectedSum);
}

TEST(EngUnitsTests, StrongTArithmeticSubtract) {
  enum class Waffles : uint8_t { kBelgian, kToaster, kPotato, kStroop };
  using ToasterT = lob::StrongT<Waffles, Waffles::kToaster, int>;
  const int kTest1 = 900;
  const int kTest2 = 600;
  constexpr auto kExpectedDifference = kTest1 - kTest2;
  const ToasterT kPlate1(kTest1);
  const ToasterT kPlate2(kTest2);
  EXPECT_EQ((kPlate1 - kPlate2).Value(), kExpectedDifference);
  EXPECT_EQ((kPlate1 - kTest2).Value(), kExpectedDifference);
  ToasterT plate3 = kPlate1;
  plate3 -= kPlate2;
  EXPECT_EQ(plate3.Value(), kExpectedDifference);
  plate3 = kPlate1;
  plate3 -= kTest2;
  EXPECT_EQ(plate3.Value(), kExpectedDifference);
}

TEST(EngUnitsTests, StrongTArithmeticMultiply) {
  enum class Waffles : uint8_t { kBelgian, kToaster, kPotato, kStroop };
  using PotatoT = lob::StrongT<Waffles, Waffles::kPotato, int>;
  const int kTest1 = 25;
  const int kTest2 = -44;
  constexpr auto kExpectedProduct = kTest1 * kTest2;
  const PotatoT kPlate1(kTest1);
  const PotatoT kPlate2(kTest2);
  EXPECT_EQ((kPlate1 * kPlate2).Value(), kExpectedProduct);
  EXPECT_EQ((kPlate1 * kTest2).Value(), kExpectedProduct);
  PotatoT plate3 = kPlate1;
  plate3 *= kPlate2;
  EXPECT_EQ(plate3.Value(), kExpectedProduct);
  plate3 = kPlate1;
  plate3 *= kTest2;
  EXPECT_EQ(plate3.Value(), kExpectedProduct);
}

TEST(EngUnitsTests, StrongTArithmeticDivide) {
  enum class Waffles : uint8_t { kBelgian, kToaster, kPotato, kStroop };
  using StroopT = lob::StrongT<Waffles, Waffles::kStroop, int>;
  const int kTest1 = -99;
  const int kTest2 = 3;
  constexpr auto kExpectedQuotient = kTest1 / kTest2;
  const StroopT kPlate1(kTest1);
  const StroopT kPlate2(kTest2);
  EXPECT_EQ((kPlate1 / kPlate2).Value(), kExpectedQuotient);
  EXPECT_EQ((kPlate1 / kTest2).Value(), kExpectedQuotient);
  StroopT plate3 = kPlate1;
  plate3 /= kPlate2;
  EXPECT_EQ(plate3.Value(), kExpectedQuotient);
  plate3 = kPlate1;
  plate3 /= kTest2;
  EXPECT_EQ(plate3.Value(), kExpectedQuotient);
}

TEST(EngUnitsTests, StrongTComparisons) {
  enum class Money : uint8_t { kDollar, kPeso };
  using DollarT = lob::StrongT<Money, Money::kDollar, int>;
  constexpr auto kTestValue1 = 1'000;
  constexpr auto kTestValue2 = 100;
  const DollarT kValue1(kTestValue1);
  const DollarT kValue2(kTestValue2);
  EXPECT_TRUE(kValue1 > kValue2);
  EXPECT_FALSE(kValue2 > kValue1);
  EXPECT_TRUE(kValue2 < kValue1);
  EXPECT_FALSE(kValue1 < kValue2);
  EXPECT_TRUE(kValue1 >= kValue2);
  EXPECT_FALSE(kValue2 >= kValue1);
  EXPECT_TRUE(kValue1 >= DollarT(kValue1));
  EXPECT_FALSE(kValue1 <= kValue2);
  EXPECT_TRUE(kValue2 <= kValue1);
  EXPECT_TRUE(kValue1 <= DollarT(kValue1));
  EXPECT_TRUE(kValue1 == DollarT(kValue1));
  EXPECT_FALSE(kValue1 == kValue2);
  EXPECT_TRUE(kValue1 != kValue2);
  EXPECT_FALSE(kValue1 != DollarT(kValue1));
}

TEST(EngUnitsTests, isnan) {
  using TestType = lob::DegreesT;  // arbitrary type
  EXPECT_TRUE(std::isnan(TestType(std::numeric_limits<double>::quiet_NaN())));
  EXPECT_FALSE(std::isnan(TestType(90)));
}

TEST(EngUnitsTests, Sqrt) {
  using TestType = lob::KgT;  // arbitrary type
  EXPECT_DOUBLE_EQ(sqrt(TestType(4.0)).Value(), 2.0);
  EXPECT_DOUBLE_EQ(sqrt(TestType(9.0)).Value(), 3.0);
  EXPECT_TRUE(std::isnan(sqrt(TestType(-1.0)).Value()));
}

TEST(EngUnitsTests, PowWithDoubleExponent) {
  using TestType = lob::PsiT;  // arbitrary type
  EXPECT_DOUBLE_EQ(pow(TestType(2.0), 3.0).Value(), 8.0);
  EXPECT_DOUBLE_EQ(pow(TestType(3.0), 2.0).Value(), 9.0);
  EXPECT_DOUBLE_EQ(pow(TestType(0.0), 0.0).Value(), 1.0);
}

TEST(EngUnitsTests, PowWithStrongTExponent) {
  using TestType = lob::FpsT;  // arbitrary type
  EXPECT_DOUBLE_EQ(pow(TestType(2.0), TestType(3.0)).Value(), 8.0);
  EXPECT_DOUBLE_EQ(pow(TestType(3.0), TestType(2.0)).Value(), 9.0);
  EXPECT_DOUBLE_EQ(pow(TestType(0.0), TestType(0.0)).Value(), 1.0);
}

TEST(EngUnitsTests, Sin) {
  using TestType = lob::KgsmT;  // arbitrary type
  const double kTestPi = std::acos(-1);
  EXPECT_NEAR(sin(TestType(kTestPi / 2)).Value(), 1.0, 1e-10);
  EXPECT_DOUBLE_EQ(sin(TestType(0.0)).Value(), 0.0);
}

TEST(EngUnitsTests, Cos) {
  using TestType = lob::InchT;  // arbitrary type
  const double kTestPi = std::acos(-1);
  EXPECT_NEAR(cos(TestType(kTestPi)).Value(), -1.0, 1e-10);
  EXPECT_DOUBLE_EQ(cos(TestType(0.0)).Value(), 1.0);
}

TEST(EngUnitsTests, Tan) {
  using TestType = lob::LbsPerCuFtT;  // arbitrary type
  const double kTestPi = std::acos(-1);
  EXPECT_NEAR(tan(TestType(kTestPi / 4)).Value(), 1.0, 1e-10);
  EXPECT_DOUBLE_EQ(tan(TestType(0.0)).Value(), 0.0);
}

TEST(EngUnitsTests, Asin) {
  using TestType = lob::RadiansT;  // arbitrary type
  EXPECT_NEAR(asin(TestType(0.0)).Value(), 0.0, 1e-10);
  EXPECT_NEAR(asin(TestType(1.0)).Value(), std::sin(1.0), 1e-10);
  EXPECT_NEAR(asin(TestType(-1.0)).Value(), std::sin(-1.0), 1e-10);
}

TEST(EngUnitsTests, Acos) {
  using TestType = lob::MphT;  // arbitrary type
  EXPECT_NEAR(acos(TestType(1.0)).Value(), std::cos(1.0), 1e-10);
  EXPECT_NEAR(acos(TestType(-1.0)).Value(), std::cos(-1.0), 1e-10);
  EXPECT_NEAR(acos(TestType(0.0)).Value(), std::cos(0.0), 1e-10);
}

TEST(EngUnitsTests, Atan) {
  using TestType = lob::FtLbsT;  // arbitrary type
  EXPECT_NEAR(atan(TestType(0.0)).Value(), 0.0, 1e-10);
  EXPECT_NEAR(atan(TestType(1.0)).Value(), std::tan(1.0), 1e-10);
  EXPECT_NEAR(atan(TestType(-1.0)).Value(), std::tan(-1.0), 1e-10);
}

TEST(EngUnitsTests, Min) {
  using TestType = lob::MeterT;  // arbitrary type
  EXPECT_DOUBLE_EQ(min(TestType(1.0), TestType(2.0)).Value(), 1.0);
  EXPECT_DOUBLE_EQ(min(TestType(-1.0), TestType(1.0)).Value(), -1.0);
}

TEST(EngUnitsTests, Max) {
  using TestType = lob::SecT;  // arbitrary type
  EXPECT_DOUBLE_EQ(max(TestType(1.0), TestType(2.0)).Value(), 2.0);
  EXPECT_DOUBLE_EQ(max(TestType(-1.0), TestType(1.0)).Value(), 1.0);
}

TEST(EngUnitsTests, AngleConversions) {
  constexpr double kTestValueDeg = 180.0;
  constexpr double kTestValueMoa = kTestValueDeg * 60;
  constexpr double kTestValueRad = 3.14159265358979323846;
  constexpr double kTestValueMil = kTestValueRad * 1'000;
  constexpr double kTestValueIphy = kTestValueMoa * 1.047;
  EXPECT_DOUBLE_EQ(lob::DegreesT(lob::MoaT(kTestValueMoa)).Value(),
                   kTestValueDeg);
  EXPECT_DOUBLE_EQ(lob::DegreesT(lob::RadiansT(kTestValueRad)).Value(),
                   kTestValueDeg);
  EXPECT_DOUBLE_EQ(lob::DegreesT(lob::MilT(kTestValueMil)).Value(),
                   kTestValueDeg);
  EXPECT_DOUBLE_EQ(lob::DegreesT(lob::IphyT(kTestValueIphy)).Value(),
                   kTestValueDeg);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::DegreesT(kTestValueDeg)).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::RadiansT(kTestValueRad)).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::MilT(kTestValueMil)).Value(), kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::IphyT(kTestValueIphy)).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::RadiansT(lob::DegreesT(kTestValueDeg)).Value(),
                   kTestValueRad);
  EXPECT_DOUBLE_EQ(lob::RadiansT(lob::MoaT(kTestValueMoa)).Value(),
                   kTestValueRad);
  EXPECT_DOUBLE_EQ(lob::RadiansT(lob::MilT(kTestValueMil)).Value(),
                   kTestValueRad);
  EXPECT_DOUBLE_EQ(lob::RadiansT(lob::IphyT(kTestValueIphy)).Value(),
                   kTestValueRad);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::DegreesT(kTestValueDeg)).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::MoaT(kTestValueMoa)).Value(), kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::RadiansT(kTestValueRad)).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::IphyT(kTestValueIphy)).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::IphyT(lob::DegreesT(kTestValueDeg)).Value(),
                   kTestValueIphy);
  EXPECT_DOUBLE_EQ(lob::IphyT(lob::MoaT(kTestValueMoa)).Value(),
                   kTestValueIphy);
  EXPECT_DOUBLE_EQ(lob::IphyT(lob::RadiansT(kTestValueRad)).Value(),
                   kTestValueIphy);
  EXPECT_DOUBLE_EQ(lob::IphyT(lob::MilT(kTestValueMil)).Value(),
                   kTestValueIphy);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(
      lob::DegreesT(lob::MoaT(lob::DegreesT(kTestValueDeg))).Value(),
      kTestValueDeg);
  EXPECT_DOUBLE_EQ(
      lob::DegreesT(lob::RadiansT(lob::DegreesT(kTestValueDeg))).Value(),
      kTestValueDeg);
  EXPECT_DOUBLE_EQ(
      lob::DegreesT(lob::MilT(lob::DegreesT(kTestValueDeg))).Value(),
      kTestValueDeg);
  EXPECT_DOUBLE_EQ(
      lob::DegreesT(lob::IphyT(lob::DegreesT(kTestValueDeg))).Value(),
      kTestValueDeg);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::DegreesT(lob::MoaT(kTestValueMoa))).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::RadiansT(lob::MoaT(kTestValueMoa))).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::MilT(lob::MoaT(kTestValueMoa))).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(lob::MoaT(lob::IphyT(lob::MoaT(kTestValueMoa))).Value(),
                   kTestValueMoa);
  EXPECT_DOUBLE_EQ(
      lob::RadiansT(lob::DegreesT(lob::RadiansT(kTestValueRad))).Value(),
      kTestValueRad);
  EXPECT_DOUBLE_EQ(
      lob::RadiansT(lob::MoaT(lob::RadiansT(kTestValueRad))).Value(),
      kTestValueRad);
  EXPECT_DOUBLE_EQ(
      lob::RadiansT(lob::MilT(lob::RadiansT(kTestValueRad))).Value(),
      kTestValueRad);
  EXPECT_DOUBLE_EQ(
      lob::RadiansT(lob::IphyT(lob::RadiansT(kTestValueRad))).Value(),
      kTestValueRad);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::DegreesT(lob::MilT(kTestValueMil))).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::MoaT(lob::MilT(kTestValueMil))).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::RadiansT(lob::MilT(kTestValueMil))).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(lob::MilT(lob::IphyT(lob::MilT(kTestValueMil))).Value(),
                   kTestValueMil);
  EXPECT_DOUBLE_EQ(
      lob::IphyT(lob::DegreesT(lob::IphyT(kTestValueIphy))).Value(),
      kTestValueIphy);
  EXPECT_DOUBLE_EQ(lob::IphyT(lob::MoaT(lob::IphyT(kTestValueIphy))).Value(),
                   kTestValueIphy);
  EXPECT_DOUBLE_EQ(
      lob::IphyT(lob::RadiansT(lob::IphyT(kTestValueIphy))).Value(),
      kTestValueIphy);
  EXPECT_DOUBLE_EQ(lob::IphyT(lob::MilT(lob::IphyT(kTestValueIphy))).Value(),
                   kTestValueIphy);
}

TEST(EngUnitsTests, EnergyConversions) {
  constexpr double kTestValueFtLb = 1.0;
  constexpr double kTestValueJ = 1.3558179483;
  const lob::JouleT kTestEnergy1 = lob::FtLbsT(kTestValueFtLb);
  const lob::FtLbsT kTestEnergy2 = lob::JouleT(kTestValueJ);
  EXPECT_DOUBLE_EQ(kTestEnergy1.Value(), kTestValueJ);
  EXPECT_DOUBLE_EQ(kTestEnergy2.Value(), kTestValueFtLb);
  EXPECT_DOUBLE_EQ(lob::FtLbsT(kTestEnergy1).Value(), kTestEnergy2.Value());
  EXPECT_DOUBLE_EQ(lob::JouleT(kTestEnergy2).Value(), kTestEnergy1.Value());
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::JouleT(lob::FtLbsT(kTestEnergy1)).Value(),
                   kTestEnergy1.Value());
  EXPECT_DOUBLE_EQ(lob::FtLbsT(lob::JouleT(kTestEnergy2)).Value(),
                   kTestEnergy2.Value());
}

TEST(EngUnitsTests, LengthConversions) {
  constexpr double kTestValueFeet = 1.0;
  constexpr double kTestValueInch = 12.0;
  constexpr double kTestValueYard = 1.0 / 3.0;
  constexpr double kTestValueMm = 304.8;
  constexpr double kTestValueCm = kTestValueMm / 10;
  constexpr double kTestValueMeter = kTestValueMm / 1000;
  EXPECT_DOUBLE_EQ(lob::InchT(lob::FeetT(kTestValueFeet)).Value(),
                   kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::MmT(kTestValueMm)).Value(), kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::CmT(kTestValueCm)).Value(), kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::InchT(kTestValueInch)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::YardT(kTestValueYard)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MmT(kTestValueMm)).Value(), kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::CmT(kTestValueCm)).Value(), kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MeterT(kTestValueMeter)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::YardT(lob::FeetT(kTestValueFeet)).Value(),
                   kTestValueYard);
  EXPECT_DOUBLE_EQ(lob::MmT(lob::FeetT(kTestValueFeet)).Value(), kTestValueMm);
  EXPECT_DOUBLE_EQ(lob::CmT(lob::FeetT(kTestValueFeet)).Value(), kTestValueCm);
  EXPECT_DOUBLE_EQ(lob::MeterT(lob::FeetT(kTestValueFeet)).Value(),
                   kTestValueMeter);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::InchT(lob::FeetT(lob::InchT(kTestValueInch))).Value(),
                   kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::YardT(lob::FeetT(lob::YardT(kTestValueYard))).Value(),
                   kTestValueYard);
  EXPECT_DOUBLE_EQ(lob::MmT(lob::FeetT(lob::MmT(kTestValueMm))).Value(),
                   kTestValueMm);
  EXPECT_DOUBLE_EQ(lob::CmT(lob::FeetT(lob::CmT(kTestValueCm))).Value(),
                   kTestValueCm);
  EXPECT_DOUBLE_EQ(
      lob::MeterT(lob::FeetT(lob::MeterT(kTestValueMeter))).Value(),
      kTestValueMeter);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::InchT(lob::FeetT(kTestValueFeet))).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::YardT(lob::FeetT(kTestValueFeet))).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MmT(lob::FeetT(kTestValueFeet))).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::CmT(lob::FeetT(kTestValueFeet))).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MeterT(lob::FeetT(kTestValueFeet))).Value(),
                   kTestValueFeet);
}

TEST(EngUnitsTests, PressureConversions) {
  constexpr double kTestValueInHg = 1.0;
  constexpr double kTestValuePa = 1 / 0.000295299801647;
  constexpr double kTestValueMillibar = kTestValuePa / 100;
  constexpr double kTestValuePsi = 1 / 2.03602128864;
  EXPECT_DOUBLE_EQ(lob::InHgT(lob::PsiT(kTestValuePsi)).Value(),
                   kTestValueInHg);
  EXPECT_DOUBLE_EQ(lob::InHgT(lob::PaT(kTestValuePa)).Value(), kTestValueInHg);
  EXPECT_DOUBLE_EQ(lob::InHgT(lob::MbarT(kTestValueMillibar)).Value(),
                   kTestValueInHg);
}

TEST(EngUnitsTests, MassConversions) {
  constexpr double kTestValueLbs = 1.0;
  constexpr double kTestValueSlug = 1 / 32.17405;
  constexpr double kTestValueGrain = 7000.0;
  constexpr double kTestValueKg = 1 / 2.204623;
  constexpr double kTestValueGram = kTestValueKg * 1000;
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::GrainT(kTestValueGrain)).Value(),
                   kTestValueLbs);
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::GramT(kTestValueGram)).Value(),
                   kTestValueLbs);
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::KgT(kTestValueKg)).Value(), kTestValueLbs);
  EXPECT_DOUBLE_EQ(lob::GrainT(lob::LbsT(kTestValueLbs)).Value(),
                   kTestValueGrain);
  EXPECT_DOUBLE_EQ(lob::SlugT(lob::LbsT(kTestValueLbs)).Value(),
                   kTestValueSlug);
  EXPECT_DOUBLE_EQ(lob::SlugT(lob::GrainT(kTestValueGrain)).Value(),
                   kTestValueSlug);
}

TEST(EngUnitsTests, SectionalDensityConversions) {
  constexpr double kTestValueKgsm = 1.0;
  constexpr double kTestValuePmsi = 703.069579639;
  EXPECT_DOUBLE_EQ(lob::PmsiT(lob::KgsmT(kTestValueKgsm)).Value(),
                   kTestValuePmsi);
  EXPECT_DOUBLE_EQ(lob::KgsmT(lob::PmsiT(kTestValuePmsi)).Value(),
                   kTestValueKgsm);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::KgsmT(lob::PmsiT(lob::KgsmT(kTestValueKgsm))).Value(),
                   kTestValueKgsm);
  EXPECT_DOUBLE_EQ(lob::PmsiT(lob::KgsmT(lob::PmsiT(kTestValuePmsi))).Value(),
                   kTestValuePmsi);
}

TEST(EngUnitsTests, SpeedConversions) {
  constexpr double kTestValueFps = 1.0;
  constexpr double kTestValueMph = 1.0 / 1.46666667;
  constexpr double kTestValueMps = 0.3048;
  constexpr double kTestValueKph = 1.0 / 0.91134442;
  constexpr double kTestValueKn = 1.0 / 1.6878099;
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::MphT(kTestValueMph)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::MpsT(kTestValueMps)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::KphT(kTestValueKph)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::KnT(kTestValueKn)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::MpsT(lob::FpsT(kTestValueFps)).Value(), kTestValueMps);
  EXPECT_DOUBLE_EQ(lob::MphT(lob::FpsT(kTestValueFps)).Value(), kTestValueMph);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::MpsT(lob::FpsT(kTestValueFps))).Value(),
                   kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::MphT(lob::FpsT(kTestValueFps))).Value(),
                   kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::MpsT(lob::FpsT(lob::MpsT(kTestValueMps))).Value(),
                   kTestValueMps);
  EXPECT_DOUBLE_EQ(lob::MphT(lob::FpsT(lob::MphT(kTestValueMph))).Value(),
                   kTestValueMph);
}

TEST(EngUnitsTests, TemperatureConversions) {
  constexpr double kTestValueDegC = 0.0;
  constexpr double kTestValueDegF = 32.0;
  constexpr double kTestValueDegR = kTestValueDegF + 459.67;
  constexpr double kTestValueDegK = 273.15;
  EXPECT_DOUBLE_EQ(lob::DegFT(lob::DegCT(kTestValueDegC)).Value(),
                   kTestValueDegF);
  EXPECT_DOUBLE_EQ(lob::DegCT(lob::DegFT(kTestValueDegF)).Value(),
                   kTestValueDegC);
  EXPECT_DOUBLE_EQ(lob::DegRT(lob::DegFT(kTestValueDegF)).Value(),
                   kTestValueDegR);
  EXPECT_DOUBLE_EQ(lob::DegFT(lob::DegRT(kTestValueDegR)).Value(),
                   kTestValueDegF);
  EXPECT_DOUBLE_EQ(lob::DegRT(lob::DegKT(kTestValueDegK)).Value(),
                   (kTestValueDegR));
  EXPECT_DOUBLE_EQ(lob::DegKT(lob::DegRT(kTestValueDegR)).Value(),
                   (kTestValueDegK));
  EXPECT_DOUBLE_EQ(lob::DegFT(lob::DegKT(kTestValueDegK)).Value(),
                   kTestValueDegF);
  EXPECT_DOUBLE_EQ(lob::DegKT(lob::DegFT(kTestValueDegF)).Value(),
                   kTestValueDegK);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::DegCT(lob::DegFT(lob::DegCT(kTestValueDegC))).Value(),
                   kTestValueDegC);
  EXPECT_DOUBLE_EQ(lob::DegRT(lob::DegFT(lob::DegRT(kTestValueDegR))).Value(),
                   kTestValueDegR);
  EXPECT_DOUBLE_EQ(lob::DegFT(lob::DegCT(lob::DegFT(kTestValueDegF))).Value(),
                   kTestValueDegF);
  EXPECT_DOUBLE_EQ(lob::DegFT(lob::DegRT(lob::DegFT(kTestValueDegF))).Value(),
                   kTestValueDegF);
}

TEST(EngUnitsTests, TimeConversions) {
  constexpr double kTestValueSec = 1;
  constexpr double kTestValueMsec = 1E3;
  constexpr double kTestValueUsec = 1E6;
  EXPECT_DOUBLE_EQ(lob::SecT(lob::MsecT(kTestValueMsec)).Value(),
                   kTestValueSec);
  EXPECT_DOUBLE_EQ(lob::SecT(lob::UsecT(kTestValueUsec)).Value(),
                   kTestValueSec);
  EXPECT_DOUBLE_EQ(lob::MsecT(lob::SecT(kTestValueSec)).Value(),
                   kTestValueMsec);
  EXPECT_DOUBLE_EQ(lob::UsecT(lob::SecT(kTestValueSec)).Value(),
                   kTestValueUsec);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::SecT(lob::UsecT(lob::SecT(kTestValueSec))).Value(),
                   kTestValueSec);
  EXPECT_DOUBLE_EQ(lob::MsecT(lob::SecT(lob::MsecT(kTestValueMsec))).Value(),
                   kTestValueMsec);
}

TEST(EngUnitsTests, TwistConversions) {
  constexpr double kTestValueInchPerTwistT = 12.0;
  constexpr double kTestValueMmPerTwistT = 304.8;
  EXPECT_DOUBLE_EQ(
      lob::InchPerTwistT(lob::MmPerTwistT(kTestValueMmPerTwistT)).Value(),
      kTestValueInchPerTwistT);
}

}  // namespace tests

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