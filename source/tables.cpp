// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#include "tables.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
#include <numeric>

#include "constants.hpp"
#include "helpers.hpp"

namespace lob {

struct Point {
  double x{0};
  double y{0};
};

struct Circle {
  Point center;
  double radius{0};
};

namespace {

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
}  // namespace

template <typename T>
double LobLerp(const T* x_lut, const T* y_lut, const size_t size,
               const double x_in) {
  if (x_in < static_cast<double>(x_lut[0])) {
    return y_lut[0];
  }

  size_t index = size - 1;

  while (index > 0 && x_in < static_cast<double>(x_lut[index])) {
    index--;
  }

  if (index == size - 1) {
    return y_lut[size - 1];
  }

  const double kX0 = x_lut[index];
  const double kX1 = x_lut[index + 1];
  const double kY0 = y_lut[index];
  const double kY1 = y_lut[index + 1];

  return ((kY1 - kY0) / (kX1 - kX0) * (x_in - kX0)) + kY0;
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
    return y_lut[0];
  }

  size_t index = size - 1;

  while (index > 0 && x_in < static_cast<double>(x_lut[index])) {
    index--;
  }

  if (index == size - 1) {
    return y_lut[size - 1];
  }

  const double kX0 = index == 0 ? x_lut[index] : x_lut[index - 1];
  const double kX1 = index == 0 ? x_lut[index + 1] : x_lut[index];
  const double kX2 = index == 0 ? x_lut[index + 2] : x_lut[index + 1];
  const double kY0 = index == 0 ? y_lut[index] : y_lut[index - 1];
  const double kY1 = index == 0 ? y_lut[index + 1] : y_lut[index];
  const double kY2 = index == 0 ? y_lut[index + 2] : y_lut[index + 1];
  const double kLerp = x_in >= kX1
                           ? ((kY2 - kY1) / (kX2 - kX1) * (x_in - kX1)) + kY1
                           : ((kY1 - kY0) / (kX1 - kX0) * (x_in - kX0)) + kY0;

  const Circle kC = FitCircle({kX0, kY0}, {kX1, kY1}, {kX2, kY2});

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

namespace {
template <typename T>
void ExpandMachDragTable(const T* pmachs, const T* pdrags, size_t old_size,
                         T* pnew_machs, T* pnew_drags, size_t new_size) {
  std::copy(pmachs, pmachs + old_size, pnew_machs);
  for (size_t i = 0; i < new_size - old_size; i++) {
    double max_diff = 0.0;
    size_t maxdex = 0;
    double max_gap = 0.0;
    size_t gapdex = 0;
    for (size_t j = 1; (j < old_size + i); j++) {
      const auto kMidPoint = (pnew_machs[j] + pnew_machs[j - 1]) / 2;
      const double kLerp =
          LobLerp(pnew_machs, pnew_drags, old_size + i, kMidPoint);
      const double kQerp =
          LobQerp(pnew_machs, pnew_drags, old_size + i, kMidPoint);
      const double kDiff = std::fabs(kLerp - kQerp);
      const T kGap = static_cast<T>(pnew_machs[j] - pnew_machs[j - 1]);
      if (kDiff > max_diff) {
        max_diff = kDiff;
        maxdex = j;
      }
      if (static_cast<double>(kGap) > max_gap) {
        max_gap = kGap;
        gapdex = j;
      }
    }
    if (maxdex == 0) {
      pnew_machs[old_size + i] =
          (pnew_machs[gapdex] + pnew_machs[gapdex - 1]) / static_cast<T>(2);
    } else {
      pnew_machs[old_size + i] =
          pnew_machs[maxdex] + pnew_machs[maxdex - 1] / static_cast<T>(2);
    }
    std::sort(pnew_machs, pnew_machs + old_size + 1U + i);
  }

  for (size_t i = 0; i < new_size; i++) {
    pnew_drags[i] =
        static_cast<T>(LobQerp(pmachs, pdrags, old_size, pnew_machs[i]));
  }
}

template void ExpandMachDragTable<uint16_t>(
    const uint16_t* pmachs, const uint16_t* pdrags, size_t old_size,
    uint16_t* pnew_machs, uint16_t* pnew_drags, size_t new_size);

template void ExpandMachDragTable<float>(const float* pmachs,
                                         const float* pdrags, size_t old_size,
                                         float* pnew_machs, float* pnew_drags,
                                         size_t new_size);

template <typename T>
void CompressMachDragTable(const T* pmachs, const T* pdrags, size_t* indices,
                           size_t old_size, T* pnew_machs, T* pnew_drags,
                           size_t new_size) {
  std::iota(indices, indices + old_size, 0);
  for (size_t i = 0; i < old_size - new_size; i++) {
    // measure the cost of replacing each point with lerp
    double min_diff = std::numeric_limits<double>::max();
    size_t mindex = old_size;
    // keep first and last points
    for (size_t j = 1; j < old_size - i - 1; j++) {
      const double kX1 = pmachs[indices[j - 1]];
      const double kX2 = pmachs[indices[j + 1]];
      const double kY1 = pdrags[indices[j - 1]];
      const double kY2 = pdrags[indices[j + 1]];
      const double kLerp = ((kY2 - kY1) / (kX2 - kX1) *
                            (static_cast<double>(pmachs[indices[j]]) - kX1)) +
                           kY1;
      const double kDiff =
          fabs(kLerp - static_cast<double>(pdrags[indices[j]]));
      if (kDiff < min_diff) {
        min_diff = kDiff;
        mindex = j;
      }
    }
    // swap most replaceable index with the end
    indices[old_size - i - 1] = mindex;
    indices[mindex] = old_size - 1;

    // sort the most irreplaceable by mach speed
    std::sort(indices, indices + old_size - i - 1,
              [pmachs](size_t a, size_t b) { return pmachs[a] < pmachs[b]; });
  }
  for (size_t i = 0; i < new_size; i++) {
    pnew_machs[i] = pmachs[indices[i]];
    pnew_drags[i] = pdrags[indices[i]];
  }
}

template void CompressMachDragTable<uint16_t>(const uint16_t* pmachs,
                                              const uint16_t* pdrags,
                                              size_t* indices, size_t old_size,
                                              uint16_t* pnew_machs,
                                              uint16_t* pnew_drags,
                                              size_t new_size);

template void CompressMachDragTable<float>(const float* pmachs,
                                           const float* pdrags, size_t* indices,
                                           size_t old_size, float* pnew_machs,
                                           float* pnew_drags, size_t new_size);
}  // namespace

template <typename T>
void ResizeMachDragTable(const T* pmachs, const T* pdrags, size_t* indices,
                         size_t old_size, T* pnew_machs, T* pnew_drags,
                         size_t new_size) {
  if (old_size == new_size) {
    std::copy(pmachs, pmachs + old_size, pnew_machs);
    std::copy(pdrags, pdrags + old_size, pnew_drags);
  } else if (old_size < new_size) {
    ExpandMachDragTable(pmachs, pdrags, old_size, pnew_machs, pnew_drags,
                        new_size);
  } else {
    CompressMachDragTable(pmachs, pdrags, indices, old_size, pnew_machs,
                          pnew_drags, new_size);
  }
}

template void ResizeMachDragTable<uint16_t>(const uint16_t* pmachs,
                                            const uint16_t* pdrags,
                                            size_t* indices, size_t old_size,
                                            uint16_t* pnew_machs,
                                            uint16_t* pnew_drags,
                                            size_t new_size);

template void ResizeMachDragTable<float>(const float* pmachs,
                                         const float* pdrags, size_t* indices,
                                         size_t old_size, float* pnew_machs,
                                         float* pnew_drags, size_t new_size);

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