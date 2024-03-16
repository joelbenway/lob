// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "eng_units.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <limits>

namespace testing {

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
  constexpr double kTestValueRad = 3.14159265358979323846;
  const lob::DegreesT kTestAngle1 = lob::DegreesT(kTestValueDeg);
  const lob::RadiansT kTestAngle2 = kTestAngle1;
  const lob::DegreesT kTestAngle3 = kTestAngle2;
  EXPECT_DOUBLE_EQ(kTestAngle1.Value(), kTestValueDeg);
  EXPECT_DOUBLE_EQ(kTestAngle2.Value(), kTestValueRad);
  EXPECT_DOUBLE_EQ(kTestAngle3.Value(), kTestValueDeg);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(
      lob::DegreesT(lob::RadiansT(lob::DegreesT(kTestValueDeg))).Value(),
      kTestValueDeg);
  EXPECT_DOUBLE_EQ(
      lob::RadiansT(lob::DegreesT(lob::RadiansT(kTestValueRad))).Value(),
      kTestValueRad);
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
  constexpr double kTestValueMeter = 0.3048;
  EXPECT_DOUBLE_EQ(lob::InchT(lob::FeetT(kTestValueFeet)).Value(),
                   kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::YardT(lob::FeetT(kTestValueFeet)).Value(),
                   kTestValueYard);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::InchT(kTestValueInch)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::YardT(kTestValueYard)).Value(),
                   kTestValueFeet);
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MmT(kTestValueMm)).Value(), kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::MmT(kTestValueMm)).Value(), kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MeterT(kTestValueMeter)).Value(),
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
  EXPECT_DOUBLE_EQ(lob::InHgT(lob::MillibarT(kTestValueMillibar)).Value(),
                   kTestValueInHg);
}

TEST(EngUnitsTests, MassConversions) {
  constexpr double kTestValueLbs = 1.0;
  constexpr double kTestValueGrain = 7000.0;
  constexpr double kTestValueKg = 1 / 2.204623;
  constexpr double kTestValueGram = kTestValueKg * 1000;
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::GrainT(kTestValueGrain)).Value(),
                   kTestValueLbs);
  EXPECT_DOUBLE_EQ(lob::GrainT(lob::LbsT(kTestValueLbs)).Value(),
                   kTestValueGrain);
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::GramT(kTestValueGram)).Value(),
                   kTestValueLbs);
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::KgT(kTestValueKg)).Value(), kTestValueLbs);
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
  EXPECT_DOUBLE_EQ(lob::DegRT(lob::DegKT(kTestValueDegK)).Value(),
                   (kTestValueDegR));
  EXPECT_DOUBLE_EQ(lob::DegCT(lob::DegFT(lob::DegCT(kTestValueDegC))).Value(),
                   kTestValueDegC);
  EXPECT_DOUBLE_EQ(lob::DegFT(lob::DegCT(lob::DegFT(kTestValueDegF))).Value(),
                   kTestValueDegF);
}

}  // namespace testing

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