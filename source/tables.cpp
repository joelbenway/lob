// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "tables.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <limits>

#include "constants.hpp"
#include "helpers.hpp"

namespace lob {
namespace help {

double CalculatePerpendicularSlope(double slope) {
  if (AreEqual(slope, 0.0)) {
    return std::numeric_limits<double>::infinity();
  }
  if (AreEqual(slope, std::numeric_limits<double>::infinity())) {
    return 0.0;
  }
  return -1.0 / slope;
}

Circle FitCircle(const Point& p1, const Point& p2, const Point& p3) {
  // Calculate the center of the circle using the perpendicular bisectors
  // method.
  const double kEpsilon = 1e-6;

  const double kCollinearity =
      ((p2.y - p1.y) * (p3.x - p2.x)) - ((p3.y - p2.y) * (p2.x - p1.x));
  if (std::abs(kCollinearity) < kEpsilon) {
    return {{0, 0}, 0};
  }

  const double kMid1x = (p1.x + p2.x) / 2;
  const double kMid1y = (p1.y + p2.y) / 2;
  const double kMid2x = (p2.x + p3.x) / 2;
  const double kMid2y = (p2.y + p3.y) / 2;
  const double kSlope1 = AreEqual((p2.x - p1.x), 0.0)
                             ? std::numeric_limits<double>::infinity()
                             : (p2.y - p1.y) / (p2.x - p1.x);
  const double kSlope2 = AreEqual((p3.x - p2.x), 0.0)
                             ? std::numeric_limits<double>::infinity()
                             : (p3.y - p2.y) / (p3.x - p2.x);
  const double kPerpendicular1 = CalculatePerpendicularSlope(kSlope1);
  const double kPerpendicular2 = CalculatePerpendicularSlope(kSlope2);
  Point center;
  if (AreEqual(kPerpendicular1, std::numeric_limits<double>::infinity())) {
    center.x = kMid1x;
    center.y = kPerpendicular2 * (center.x - kMid2x) + kMid2y;
  } else if (AreEqual(kPerpendicular2,
                      std::numeric_limits<double>::infinity())) {
    center.x = kMid2x;
    center.y = kPerpendicular1 * (center.x - kMid1x) + kMid1y;
  } else {
    center.x = (kMid2y - kMid1y + kPerpendicular1 * kMid1x -
                kPerpendicular2 * kMid2x) /
               (kPerpendicular1 - kPerpendicular2);
    center.y = kPerpendicular1 * (center.x - kMid1x) + kMid1y;
  }

  const double kRadius =
      std::sqrt(std::pow(p1.x - center.x, 2) + std::pow(p1.y - center.y, 2));

  return {center, kRadius};
}

double FindAngleToPointOnCircle(Point p, Circle c) {
  const double kAngle = std::atan2(p.y - c.center.y, p.x - c.center.x);
  if (kAngle > 2 * kPi) {
    return kAngle - (2 * kPi);
  }
  if (kAngle < 0) {
    return kAngle + (2 * kPi);
  }
  return kAngle;
}
}  // namespace help

template <typename T>
double LobLerp(const T* x_lut, const T* y_lut, const size_t size,
               const double x_in) {
  assert(!(x_in < 0.0) && "input is not negative");
  assert(x_lut != nullptr && y_lut != nullptr && "Input arrays cannot be null");

  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  const T* pupper = std::upper_bound(
      x_lut, x_lut + size, x_in, [](double val, const T& element) {
        return val < static_cast<double>(element);
      });

  const auto kDiff = std::distance(x_lut, pupper);
  const auto kIndex = static_cast<size_t>(kDiff - 1);
  assert(kIndex < size - 1 && "kIndex out of bounds");

  const auto kX0 = static_cast<double>(x_lut[kIndex]);
  const auto kX1 = static_cast<double>(x_lut[kIndex + 1]);
  const auto kY0 = static_cast<double>(y_lut[kIndex]);
  const auto kY1 = static_cast<double>(y_lut[kIndex + 1]);

  const auto kDx = kX1 - kX0;
  assert(kDx > 0.0 && "x values must be increasing");
  const double kT = (x_in - kX0) / kDx;
  return kY0 + (kT * (kY1 - kY0));
}

template double LobLerp<uint16_t>(const uint16_t* x_lut, const uint16_t* y_lut,
                                  const size_t size, const double x_in);

template double LobLerp<float>(const float* x_lut, const float* y_lut,
                               const size_t size, const double x_in);

template <typename T>
double LobQerp(const T* x_lut, const T* y_lut, const size_t size,
               const double x_in) {
  if (size < 3) {
    return LobLerp(x_lut, y_lut, size, x_in);
  }

  if (x_in < static_cast<double>(x_lut[0])) {
    return static_cast<double>(y_lut[0]);
  }

  size_t index = size - 1;

  while (index > 0 && x_in < static_cast<double>(x_lut[index])) {
    index--;
  }

  if (index == size - 1) {
    return static_cast<double>(y_lut[size - 1]);
  }

  const auto kX0 =
      static_cast<double>(index == 0 ? x_lut[index] : x_lut[index - 1]);
  const auto kX1 =
      static_cast<double>(index == 0 ? x_lut[index + 1] : x_lut[index]);
  const auto kX2 =
      static_cast<double>(index == 0 ? x_lut[index + 2] : x_lut[index + 1]);
  const auto kY0 =
      static_cast<double>(index == 0 ? y_lut[index] : y_lut[index - 1]);
  const auto kY1 =
      static_cast<double>(index == 0 ? y_lut[index + 1] : y_lut[index]);
  const auto kY2 =
      static_cast<double>(index == 0 ? y_lut[index + 2] : y_lut[index + 1]);
  const double kLerp = x_in >= kX1
                           ? ((kY2 - kY1) / (kX2 - kX1) * (x_in - kX1)) + kY1
                           : ((kY1 - kY0) / (kX1 - kX0) * (x_in - kX0)) + kY0;

  const help::Circle kC = help::FitCircle({kX0, kY0}, {kX1, kY1}, {kX2, kY2});

  const double kMinimumRadius = 0.0001;

  if (kC.radius < kMinimumRadius) {
    return kLerp;
  }

  const double kDiscriminant =
      std::pow(kC.radius, 2) - std::pow(x_in - kC.center.x, 2);

  const double kResult1 = kC.center.y + std::sqrt(kDiscriminant);
  const double kResult2 = kC.center.y - std::sqrt(kDiscriminant);

  const double kAnglePoint0 = FindAngleToPointOnCircle({kX0, kY0}, kC);
  const double kAnglePoint2 = FindAngleToPointOnCircle({kX2, kY2}, kC);
  const double kAngleResult1 = FindAngleToPointOnCircle({x_in, kResult1}, kC);
  const double kAngleResult2 = FindAngleToPointOnCircle({x_in, kResult2}, kC);

  if (kAngleResult1 >= std::min(kAnglePoint0, kAnglePoint2) &&
      kAngleResult1 <= std::max(kAnglePoint0, kAnglePoint2)) {
    return kResult1;
  }

  if (kAngleResult2 >= std::min(kAnglePoint0, kAnglePoint2) &&
      kAngleResult2 <= std::max(kAnglePoint0, kAnglePoint2)) {
    return kResult2;
  }
  return kLerp;
}

template double LobQerp<uint16_t>(const uint16_t* x_lut, const uint16_t* y_lut,
                                  const size_t size, const double x_in);

template double LobQerp<float>(const float* x_lut, const float* y_lut,
                               const size_t size, const double x_in);
}  // namespace lob

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