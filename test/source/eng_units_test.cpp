// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "eng_units.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace tests {

using TestT = lob::FeetT;  // arbitrary type

TEST(EngUnitsTests, Constructor) {
  std::unique_ptr<TestT> ptest = nullptr;
  const double kValue = 100.0;
  ptest = std::make_unique<TestT>(kValue);
  ASSERT_NE(ptest, nullptr);
  EXPECT_DOUBLE_EQ(ptest->Value(), kValue);
  ptest.reset();
}

TEST(EngUnitsTests, ConstructorConversion) {
  using lob::CaliberT;
  using lob::InchT;
  std::unique_ptr<CaliberT> ptest = nullptr;
  const InchT kA(3.0);
  const InchT kB(1.0 / 0.308);
  const double kExpected = kA.Value() * kB.Value();
  ptest = std::make_unique<CaliberT>(kA, kB.Value());
  ASSERT_NE(ptest, nullptr);
  EXPECT_DOUBLE_EQ(ptest->Value(), kExpected);
  ptest.reset();
  ptest = std::make_unique<CaliberT>(kA, kB);
  ASSERT_NE(ptest, nullptr);
  EXPECT_DOUBLE_EQ(ptest->Value(), kExpected);
  ptest.reset();
}

TEST(EngUnitsTests, CopyConstructor) {
  const TestT kA = TestT(100.0);
  const auto kB = kA;
  EXPECT_DOUBLE_EQ(kA.Value(), kB.Value());
}

TEST(EngUnitsTests, MoveConstructor) {
  const double kValue = 100.0;
  TestT a = TestT(kValue);
  const auto kB = std::move(a);
  EXPECT_DOUBLE_EQ(kB.Value(), kValue);
}

TEST(EngUnitsTests, CopyAssignmentOperator) {
  const TestT kA = TestT(100.0);
  const double kB = 0.0;
  TestT b(kB);
  b = b;
  EXPECT_DOUBLE_EQ(b.Value(), kB);
  b = kA;
  EXPECT_DOUBLE_EQ(kA.Value(), b.Value());
}

TEST(EngUnitsTests, MoveAssignmentOperator) {
  const double kValue = 100.0;
  TestT a = TestT(kValue);
  const double kB = 0.0;
  TestT b(kB);
  b = std::move(b);
  EXPECT_DOUBLE_EQ(b.Value(), kB);
  b = std::move(a);
  EXPECT_DOUBLE_EQ(b.Value(), kValue);
}

TEST(EngUnitTests, AdditionOperator) {
  const auto kA = TestT(100);
  const auto kB = TestT(50);
  const auto kC = TestT(150);
  EXPECT_EQ(kA + kB, kC);
  EXPECT_EQ(kA + kB.Value(), kC);
}

TEST(EngUnitTests, SubtractionOperator) {
  const auto kA = TestT(100);
  const auto kB = TestT(50);
  const auto kC = TestT(150);
  EXPECT_EQ(kC - kB, kA);
  EXPECT_EQ(kC - kB.Value(), kA);
}

TEST(EngUnitTests, MultiplicationOperator) {
  const auto kA = TestT(100);
  const auto kB = TestT(50);
  const auto kC = TestT(5000);
  EXPECT_EQ(kA * kB, kC);
  EXPECT_EQ(kA * kB.Value(), kC);
}

TEST(EngUnitTests, DivisionOperator) {
  const auto kA = TestT(100);
  const auto kB = TestT(50);
  const auto kC = TestT(5000);
  EXPECT_EQ(kC / kB, kA);
  EXPECT_EQ(kC / kB.Value(), kA);
}

TEST(EngUnitTests, ModuloOperator) {
  const auto kA = TestT(100);
  const auto kB = TestT(3);
  const auto kC = TestT(1);
  EXPECT_EQ(kA % kB, kC);
  EXPECT_EQ(kA % kB.Value(), kC);
  EXPECT_EQ(kA % 0, TestT(std::numeric_limits<double>::quiet_NaN()));
}

TEST(EngUnitTests, AdditionAssignmentOperator) {
  const auto kA = TestT(50);
  auto b = kA;
  b += kA;
  EXPECT_EQ(b, kA + kA);
  b += kA.Value();
  EXPECT_EQ(b, kA + kA + kA);
}

TEST(EngUnitTests, SubtractionAssignmentOperator) {
  const auto kA = TestT(50);
  auto b = kA;
  b -= kA;
  EXPECT_EQ(b, kA - kA);
  b -= kA.Value();
  EXPECT_EQ(b, kA - kA - kA);
}

TEST(EngUnitTests, MultiplicationAssignmentOperator) {
  const auto kA = TestT(5);
  auto b = kA;
  b *= kA;
  EXPECT_EQ(b, kA * kA);
  b *= kA.Value();
  EXPECT_EQ(b, kA * kA * kA);
}

TEST(EngUnitTests, DivisionAssignmentOperator) {
  const auto kA = TestT(50);
  auto b = kA;
  b /= kA;
  EXPECT_EQ(b, kA / kA);
  b /= kA.Value();
  EXPECT_EQ(b, kA / kA / kA);
}

TEST(EngUnitTests, IncrementOperators) {
  const TestT kNum(1);
  auto a = kNum;
  EXPECT_EQ(++a, (kNum + 1));
  EXPECT_EQ(a++, (kNum + 1));
  EXPECT_EQ(a--, (kNum + 2));
  EXPECT_EQ(a, (kNum + 1));
  EXPECT_EQ(--a, kNum);
}

TEST(EngUnitTests, IsNaN) {
  const TestT kA(5);
  const TestT kB(std::numeric_limits<double>::quiet_NaN());
  EXPECT_FALSE(kA.IsNaN());
  EXPECT_TRUE(kB.IsNaN());

  enum class Beer : uint8_t { kBottle, kCan };
  using BottleT = lob::StrongT<Beer, Beer::kBottle, uint32_t>;
  const BottleT kC(5);
  EXPECT_FALSE(kC.IsNaN());
  EXPECT_FALSE(BottleT(std::numeric_limits<uint32_t>::quiet_NaN()).IsNaN());
}

TEST(EngUnitTests, Inverse) {
  const TestT kA(5);
  const TestT kB(1E6);
  const double kC(0.2);
  const double kD(1E-6);
  EXPECT_EQ(kA.Inverse().Value(), kC);
  EXPECT_EQ(kB.Inverse().Value(), kD);
}

TEST(EngUnitTests, Float) {
  const TestT kA(std::acos(-1));
  const auto kB(static_cast<float>(std::acos(-1)));
  EXPECT_FLOAT_EQ(kA.Float(), kB);
}

TEST(EngUnitTests, U32) {
  const TestT kA(std::acos(-1));
  const auto kB(static_cast<uint32_t>(std::round(std::acos(-1))));
  EXPECT_EQ(kA.U32(), kB);
}

TEST(EngUnitTests, U16) {
  const TestT kA(std::acos(-1));
  const auto kB(static_cast<uint16_t>(std::round(std::acos(-1))));
  EXPECT_EQ(kA.U16(), kB);
}

TEST(EngUnitTests, Comparisons) {
  const auto kA = TestT(100);
  const auto kB = TestT(100);
  const auto kC = TestT(100 - 1e-10);
  EXPECT_TRUE(kA == kB);
  EXPECT_FALSE(kA == kC);
  EXPECT_FALSE(kA != kB);
  EXPECT_TRUE(kA != kC);
  EXPECT_TRUE(kA >= kB);
  EXPECT_TRUE(kA >= kC);
  EXPECT_FALSE(kA > kB);
  EXPECT_TRUE(kA > kC);
  EXPECT_TRUE(kA <= kB);
  EXPECT_FALSE(kA <= kC);
  EXPECT_FALSE(kA < kB);
  EXPECT_FALSE(kA < kC);
}

TEST(EngUnitsTests, isnan) {
  EXPECT_TRUE(std::isnan(TestT(std::numeric_limits<double>::quiet_NaN())));
  EXPECT_FALSE(std::isnan(TestT(90)));
}

TEST(EngUnitsTests, sqrt) {
  const auto kA = TestT(9);
  const auto kB = TestT(3);
  EXPECT_TRUE(std::sqrt(TestT(kA)) == kB);
  EXPECT_DOUBLE_EQ(std::sqrt(kA).Value(), std::sqrt(kA.Value()));
  EXPECT_TRUE(std::isnan(std::sqrt(TestT(-1.0)).Value()));
}

TEST(EngUnitsTests, pow) {
  const auto kA = TestT(2);
  const auto kB = TestT(3);
  EXPECT_TRUE(std::pow(kA, kB.Value()) == pow(kA, kB));
  EXPECT_DOUBLE_EQ(std::pow(kA, kB).Value(), std::pow(kA.Value(), kB.Value()));
}

TEST(EngUnitsTests, sin) {
  const auto kA = TestT(std::acos(-1));
  EXPECT_DOUBLE_EQ(std::sin(kA).Value(), std::sin(kA.Value()));
}

TEST(EngUnitsTests, cos) {
  const auto kA = TestT(std::acos(-1));
  EXPECT_DOUBLE_EQ(std::cos(kA).Value(), std::cos(kA.Value()));
}

TEST(EngUnitsTests, tan) {
  const auto kA = TestT(std::acos(-1) / 4);
  EXPECT_DOUBLE_EQ(std::tan(kA).Value(), std::tan(kA.Value()));
}

TEST(EngUnitsTests, asin) {
  const auto kA = TestT(1);
  EXPECT_DOUBLE_EQ(std::asin(kA).Value(), std::asin(kA.Value()));
  EXPECT_EQ(std::asin(std::sin(kA)), kA);
}

TEST(EngUnitsTests, acos) {
  const auto kA = TestT(1);
  EXPECT_DOUBLE_EQ(std::acos(kA).Value(), std::acos(kA.Value()));
  EXPECT_EQ(std::acos(std::cos(kA)), kA);
}

TEST(EngUnitsTests, atan) {
  const auto kA = TestT(1);
  EXPECT_DOUBLE_EQ(std::atan(kA).Value(), std::atan(kA.Value()));
  EXPECT_EQ(std::atan(std::tan(kA)), kA);
}

TEST(EngUnitsTests, min) {
  const auto kA = TestT(1);
  const auto kB = TestT(-1);
  EXPECT_DOUBLE_EQ(std::min(kA, kB).Value(), std::min(kA.Value(), kB.Value()));
}

TEST(EngUnitsTests, max) {
  const auto kA = TestT(1);
  const auto kB = TestT(-1);
  EXPECT_DOUBLE_EQ(std::max(kA, kB).Value(), std::max(kA.Value(), kB.Value()));
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

TEST(EngUnitsTests, AreaConversions) {
  constexpr double kTestValueSqFt = 1.0;
  constexpr double kTestValueSqIn = 144.0;
  EXPECT_DOUBLE_EQ(lob::SqFtT(lob::SqInT(kTestValueSqIn)).Value(),
                   kTestValueSqFt);
  EXPECT_DOUBLE_EQ(lob::SqInT(lob::SqFtT(kTestValueSqFt)).Value(),
                   kTestValueSqIn);
}

TEST(EngUnitsTests, DensityConversions) {
  constexpr double kTestValueLbsPerCuFt = 1.0;
  constexpr double kTestValueGrPerCuIn = 4.0509259259259256;
  EXPECT_DOUBLE_EQ(
      lob::LbsPerCuFtT(lob::GrPerCuInT(kTestValueGrPerCuIn)).Value(),
      kTestValueLbsPerCuFt);
  EXPECT_DOUBLE_EQ(
      lob::GrPerCuInT(lob::LbsPerCuFtT(kTestValueLbsPerCuFt)).Value(),
      kTestValueGrPerCuIn);
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
  EXPECT_DOUBLE_EQ(lob::YardT(lob::FeetT(kTestValueFeet)).Value(),
                   kTestValueYard);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::InchT(kTestValueInch)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::MmT(lob::InchT(kTestValueInch)).Value(), kTestValueMm);
  EXPECT_DOUBLE_EQ(lob::CmT(lob::InchT(kTestValueInch)).Value(), kTestValueCm);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::YardT(kTestValueYard)).Value(),
                   kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::YardT(kTestValueYard)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::MeterT(lob::YardT(kTestValueYard)).Value(),
                   kTestValueMeter);
  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MmT(kTestValueMm)).Value(), kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::MmT(kTestValueMm)).Value(), kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::CmT(kTestValueCm)).Value(), kTestValueInch);
  EXPECT_DOUBLE_EQ(lob::InchT(lob::MeterT(kTestValueMeter)).Value(),
                   kTestValueInch);

  EXPECT_DOUBLE_EQ(lob::FeetT(lob::MeterT(kTestValueMeter)).Value(),
                   kTestValueFeet);
  EXPECT_DOUBLE_EQ(lob::YardT(lob::MeterT(kTestValueMeter)).Value(),
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
  EXPECT_DOUBLE_EQ(lob::PsiT(lob::InHgT(kTestValueInHg)).Value(),
                   kTestValuePsi);
  EXPECT_DOUBLE_EQ(lob::PaT(lob::InHgT(kTestValueInHg)).Value(), kTestValuePa);
  EXPECT_DOUBLE_EQ(lob::MbarT(lob::InHgT(kTestValueInHg)).Value(),
                   kTestValueMillibar);
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
  EXPECT_DOUBLE_EQ(lob::SlugT(lob::GrainT(kTestValueGrain)).Value(),
                   kTestValueSlug);
  EXPECT_DOUBLE_EQ(lob::GramT(lob::GrainT(kTestValueGrain)).Value(),
                   kTestValueGram);
  EXPECT_DOUBLE_EQ(lob::KgT(lob::GrainT(kTestValueGrain)).Value(),
                   kTestValueKg);
  EXPECT_DOUBLE_EQ(lob::GrainT(lob::LbsT(kTestValueLbs)).Value(),
                   kTestValueGrain);
  EXPECT_DOUBLE_EQ(lob::SlugT(lob::LbsT(kTestValueLbs)).Value(),
                   kTestValueSlug);
  EXPECT_DOUBLE_EQ(lob::LbsT(lob::GramT(kTestValueGram)).Value(),
                   kTestValueLbs);
  EXPECT_DOUBLE_EQ(lob::GrainT(lob::KgT(kTestValueKg)).Value(),
                   kTestValueGrain);
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
  EXPECT_DOUBLE_EQ(lob::MpsT(lob::FpsT(kTestValueFps)).Value(), kTestValueMps);
  EXPECT_DOUBLE_EQ(lob::MphT(lob::FpsT(kTestValueFps)).Value(), kTestValueMph);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::MphT(kTestValueMph)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::KphT(lob::MphT(kTestValueMph)).Value(), kTestValueKph);
  EXPECT_DOUBLE_EQ(lob::KnT(lob::MphT(kTestValueMph)).Value(), kTestValueKn);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::MpsT(kTestValueMps)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::KphT(kTestValueKph)).Value(), kTestValueFps);
  EXPECT_DOUBLE_EQ(lob::FpsT(lob::KnT(kTestValueKn)).Value(), kTestValueFps);
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
  // Round-trip conversions to ensure accuracy is preserved.
  EXPECT_DOUBLE_EQ(lob::SecT(lob::UsecT(lob::SecT(kTestValueSec))).Value(),
                   kTestValueSec);
  EXPECT_DOUBLE_EQ(lob::MsecT(lob::SecT(lob::MsecT(kTestValueMsec))).Value(),
                   kTestValueMsec);
}

TEST(EngUnitsTests, TwistRateConversions) {
  constexpr double kTestValueInchesPerTwist = 12;
  constexpr double kTestValueMmPerTwist = 304.8;
  EXPECT_DOUBLE_EQ(
      lob::InchPerTwistT(lob::MmPerTwistT(kTestValueMmPerTwist)).Value(),
      kTestValueInchesPerTwist);
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