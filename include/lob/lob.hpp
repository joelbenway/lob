// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "lob/lob.h"

namespace lob {

namespace detail {

template <typename T = double>
constexpr T NaN() {
  static_assert(std::is_floating_point<T>::value,
                "NaN() only supports floating-point types");
  return std::numeric_limits<T>::quiet_NaN();
}

}  // namespace detail

enum class DragFunctionT : unsigned char {
  kG1 = ::kLobDragFunctionG1,
  kG2 = ::kLobDragFunctionG2,
  kG5 = ::kLobDragFunctionG5,
  kG6 = ::kLobDragFunctionG6,
  kG7 = ::kLobDragFunctionG7,
  kG8 = ::kLobDragFunctionG8
};

enum class AtmosphereReferenceT : unsigned char {
  kArmyStandardMetro = ::kLobAtmosphereReferenceArmyStandardMetro,
  kIcao = ::kLobAtmosphereReferenceIcao
};

enum class ClockAngleT : unsigned char {
  kI = ::kLobClockAngleI,
  kII = ::kkLobClockAngleII,
  kIII = ::kkkLobClockAngleIII,
  kIV = ::kkLobClockAngleIV,
  kV = ::kLobClockAngleV,
  kVI = ::kkLobClockAngleVI,
  kVII = ::kkkLobClockAngleVII,
  kVIII = ::kkkkLobClockAngleVIII,
  kIX = ::kkLobClockAngleIX,
  kX = ::kLobClockAngleX,
  kXI = ::kkLobClockAngleXI,
  kXII = ::kkkLobClockAngleXII
};

enum class ErrorT : unsigned char {
  kNone = ::kLobErrorNone,
  kAirPressureOOR = ::kLobErrorAirPressureOOR,
  kAltitudeOfBarometerOOR = ::kLobErrorAltitudeOfBarometerOOR,
  kAltitudeOfFiringSiteOOR = ::kLobErrorAltitudeOfFiringSiteOOR,
  kAltitudeOfThermometerOOR = ::kLobErrorAltitudeOfThermometerOOR,
  kAzimuthOOR = ::kLobErrorAzimuthOOR,
  kBallisticCoefficientOOR = ::kLobErrorBallisticCoefficientOOR,
  kBallisticCoefficientRequired = ::kLobErrorBallisticCoefficientRequired,
  kBaseDiameterOOR = ::kLobErrorBaseDiameterOOR,
  kDiameterOOR = ::kLobErrorDiameterOOR,
  kHumidityOOR = ::kLobErrorHumidityOOR,
  kInitialVelocityRequired = ::kLobErrorInitialVelocityRequired,
  kInternalError = ::kLobErrorInternalError,
  kLatitudeOOR = ::kLobErrorLatitudeOOR,
  kLengthOOR = ::kLobErrorLengthOOR,
  kMassOOR = ::kLobErrorMassOOR,
  kMaximumTimeOOR = ::kLobErrorMaximumTimeOOR,
  kMeplatDiameterOOR = ::kLobErrorMeplatDiameterOOR,
  kNoseLengthOOR = ::kLobErrorNoseLengthOOR,
  kOgiveRtROOR = ::kLobErrorOgiveRtROOR,
  kRangeAngleOOR = ::kLobErrorRangeAngleOOR,
  kTailLengthOOR = ::kLobErrorTailLengthOOR,
  kWindHeadingOOR = ::kLobErrorWindHeadingOOR,
  kZeroAngleOOR = ::kLobErrorZeroAngleOOR,
  kZeroDataRequired = ::kLobErrorZeroDataRequired,
  kZeroDistanceOOR = ::kLobErrorZeroDistanceOOR,
  kNotFormed = ::kLobErrorNotFormed
};

using Gravity = ::LobGravity;
using Wind = ::LobWind;
using Coriolis = ::LobCoriolis;
using Input = ::LobInput;
using Output = ::LobOutput;

inline bool operator==(::LobErrorT a, ErrorT b) noexcept {
  return a == static_cast<::LobErrorT>(b);
}
inline bool operator!=(::LobErrorT a, ErrorT b) noexcept {
  return a != static_cast<::LobErrorT>(b);
}
inline bool operator==(ErrorT a, ::LobErrorT b) noexcept {
  return static_cast<::LobErrorT>(a) == b;
}
inline bool operator!=(ErrorT a, ::LobErrorT b) noexcept {
  return static_cast<::LobErrorT>(a) != b;
}

template <typename T = double>
constexpr T NaN() {
  return detail::NaN<T>();
}

class Builder {
 public:
  Builder() { ::LobBuilderInit(&builder_); }
  ~Builder() { ::LobBuilderDestroy(&builder_); }

  Builder(const Builder& other) {
    ::LobBuilderCopy(&builder_, &other.builder_);
  }

  Builder(Builder&& other) noexcept {
    ::LobBuilderCopy(&builder_, &other.builder_);
    ::LobBuilderReset(&other.builder_);
  }

  Builder& operator=(const Builder& rhs) {
    if (this != &rhs) {
      ::LobBuilderDestroy(&builder_);
      ::LobBuilderCopy(&builder_, &rhs.builder_);
    }
    return *this;
  }

  Builder& operator=(Builder&& rhs) noexcept {
    if (this != &rhs) {
      ::LobBuilderDestroy(&builder_);
      ::LobBuilderCopy(&builder_, &rhs.builder_);
      ::LobBuilderReset(&rhs.builder_);
    }
    return *this;
  }

  Builder& BallisticCoefficientPsi(double value) {
    ::LobBuilderBallisticCoefficientPsi(&builder_, value);
    return *this;
  }

  Builder& BCAtmosphere(AtmosphereReferenceT type) {
    ::LobBuilderBCAtmosphere(&builder_,
                             static_cast<::LobAtmosphereReferenceT>(type));
    return *this;
  }

  Builder& BCDragFunction(DragFunctionT type) {
    ::LobBuilderBCDragFunction(&builder_,
                               static_cast<::LobDragFunctionT>(type));
    return *this;
  }

  Builder& DiameterInch(double value) {
    ::LobBuilderDiameterInch(&builder_, value);
    return *this;
  }

  Builder& MeplatDiameterInch(double value) {
    ::LobBuilderMeplatDiameterInch(&builder_, value);
    return *this;
  }

  Builder& BaseDiameterInch(double value) {
    ::LobBuilderBaseDiameterInch(&builder_, value);
    return *this;
  }

  Builder& LengthInch(double value) {
    ::LobBuilderLengthInch(&builder_, value);
    return *this;
  }

  Builder& NoseLengthInch(double value) {
    ::LobBuilderNoseLengthInch(&builder_, value);
    return *this;
  }

  Builder& TailLengthInch(double value) {
    ::LobBuilderTailLengthInch(&builder_, value);
    return *this;
  }

  Builder& OgiveRtR(double value) {
    ::LobBuilderOgiveRtR(&builder_, value);
    return *this;
  }

  Builder& MachVsDragTable(const float* pmachs, const float* pdrags,
                           size_t size) {
    ::LobBuilderMachVsDragTable(&builder_, pmachs, pdrags, size);
    return *this;
  }

  template <size_t N>
  Builder& MachVsDragTable(const std::array<float, N>& machs,
                           const std::array<float, N>& drags) {
    ::LobBuilderMachVsDragTable(&builder_, machs.data(), drags.data(), N);
    return *this;
  }

  Builder& MassGrains(double value) {
    ::LobBuilderMassGrains(&builder_, value);
    return *this;
  }

  Builder& InitialVelocityFps(uint16_t value) {
    ::LobBuilderInitialVelocityFps(&builder_, value);
    return *this;
  }

  Builder& OpticHeightInches(double value) {
    ::LobBuilderOpticHeightInches(&builder_, value);
    return *this;
  }

  Builder& TwistInchesPerTurn(double value) {
    ::LobBuilderTwistInchesPerTurn(&builder_, value);
    return *this;
  }

  Builder& ZeroAngleMOA(double value) {
    ::LobBuilderZeroAngleMOA(&builder_, value);
    return *this;
  }

  Builder& ZeroDistanceYds(double value) {
    ::LobBuilderZeroDistanceYds(&builder_, value);
    return *this;
  }

  Builder& ZeroImpactHeightInches(double value) {
    ::LobBuilderZeroImpactHeightInches(&builder_, value);
    return *this;
  }

  Builder& AltitudeOfFiringSiteFt(double value) {
    ::LobBuilderAltitudeOfFiringSiteFt(&builder_, value);
    return *this;
  }

  Builder& AirPressureInHg(double value) {
    ::LobBuilderAirPressureInHg(&builder_, value);
    return *this;
  }

  Builder& AltitudeOfBarometerFt(double value) {
    ::LobBuilderAltitudeOfBarometerFt(&builder_, value);
    return *this;
  }

  Builder& TemperatureDegF(double value) {
    ::LobBuilderTemperatureDegF(&builder_, value);
    return *this;
  }

  Builder& AltitudeOfThermometerFt(double value) {
    ::LobBuilderAltitudeOfThermometerFt(&builder_, value);
    return *this;
  }

  Builder& RelativeHumidityPercent(double value) {
    ::LobBuilderRelativeHumidityPercent(&builder_, value);
    return *this;
  }

  Builder& WindHeading(ClockAngleT value) {
    ::LobBuilderWindHeading(&builder_, static_cast<::LobClockAngleT>(value));
    return *this;
  }

  Builder& WindHeadingDeg(double value) {
    ::LobBuilderWindHeadingDeg(&builder_, value);
    return *this;
  }

  Builder& WindSpeedFps(double value) {
    ::LobBuilderWindSpeedFps(&builder_, value);
    return *this;
  }

  Builder& WindSpeedMph(double value) {
    ::LobBuilderWindSpeedMph(&builder_, value);
    return *this;
  }

  Builder& AzimuthDeg(double value) {
    ::LobBuilderAzimuthDeg(&builder_, value);
    return *this;
  }

  Builder& LatitudeDeg(double value) {
    ::LobBuilderLatitudeDeg(&builder_, value);
    return *this;
  }

  Builder& RangeAngleDeg(double value) {
    ::LobBuilderRangeAngleDeg(&builder_, value);
    return *this;
  }

  Builder& MinimumSpeed(uint16_t value) {
    ::LobBuilderMinimumSpeed(&builder_, value);
    return *this;
  }

  Builder& MinimumEnergy(uint16_t value) {
    ::LobBuilderMinimumEnergy(&builder_, value);
    return *this;
  }

  Builder& MaximumTime(double value) {
    ::LobBuilderMaximumTime(&builder_, value);
    return *this;
  }

  Builder& StepSize(uint16_t value) {
    ::LobBuilderStepSize(&builder_, value);
    return *this;
  }

  Builder& Reset() noexcept {
    ::LobBuilderReset(&builder_);
    return *this;
  }

  Input Build() { return ::LobBuilderBuild(&builder_); }

 private:
  ::LobBuilder builder_;
};

inline size_t Solve(const Input& in, const uint32_t* pranges, Output* pouts,
                    size_t size) {
  return ::LobSolve(&in, pranges, pouts, size);
}

template <size_t N>
inline size_t Solve(const Input& in, const std::array<uint32_t, N>& pranges,
                    std::array<Output, N>& pouts) {
  return ::LobSolve(&in, pranges.data(), pouts.data(), N);
}

inline const char* Version() { return ::LobVersion(); }

inline double MoaToMil(double value) { return ::LobMoaToMil(value); }
inline double MoaToDeg(double value) { return ::LobMoaToDeg(value); }
inline double MoaToIphy(double value) { return ::LobMoaToIphy(value); }
inline double MoaToInch(double value, double range_ft) {
  return ::LobMoaToInch(value, range_ft);
}

inline double MilToMoa(double value) { return ::LobMilToMoa(value); }
inline double MilToDeg(double value) { return ::LobMilToDeg(value); }
inline double MilToIphy(double value) { return ::LobMilToIphy(value); }
inline double MilToInch(double value, double range_ft) {
  return ::LobMilToInch(value, range_ft);
}

inline double DegToMoa(double value) { return ::LobDegToMoa(value); }
inline double DegToMil(double value) { return ::LobDegToMil(value); }

inline double InchToMoa(double value, double range_ft) {
  return ::LobInchToMoa(value, range_ft);
}
inline double InchToMil(double value, double range_ft) {
  return ::LobInchToMil(value, range_ft);
}
inline double InchToDeg(double value, double range_ft) {
  return ::LobInchToDeg(value, range_ft);
}

inline double JToFtLbs(double value) { return ::LobJToFtLbs(value); }
inline double FtLbsToJ(double value) { return ::LobFtLbsToJ(value); }

inline double MToYd(double value) { return ::LobMToYd(value); }
inline double YdToFt(double value) { return ::LobYdToFt(value); }
inline double MToFt(double value) { return ::LobMToFt(value); }
inline double FtToIn(double value) { return ::LobFtToIn(value); }
inline double MmToIn(double value) { return ::LobMmToIn(value); }
inline double CmToIn(double value) { return ::LobCmToIn(value); }
inline double YdToM(double value) { return ::LobYdToM(value); }
inline double FtToM(double value) { return ::LobFtToM(value); }
inline double FtToYd(double value) { return ::LobFtToYd(value); }
inline double InToMm(double value) { return ::LobInToMm(value); }
inline double InToCm(double value) { return ::LobInToCm(value); }
inline double InToFt(double value) { return ::LobInToFt(value); }

inline double PaToInHg(double value) { return ::LobPaToInHg(value); }
inline double MbarToInHg(double value) { return ::LobMbarToInHg(value); }
inline double PsiToInHg(double value) { return ::LobPsiToInHg(value); }

inline double LbsToGrain(double value) { return ::LobLbsToGrain(value); }
inline double GToGrain(double value) { return ::LobGToGrain(value); }
inline double KgToGrain(double value) { return ::LobKgToGrain(value); }

inline double KgSqMToPmsi(double value) { return ::LobKgSqMToPmsi(value); }

inline double FpsToMps(double value) { return ::LobFpsToMps(value); }
inline double MpsToFps(double value) { return ::LobMpsToFps(value); }
inline double KphToMph(double value) { return ::LobKphToMph(value); }
inline double KnToMph(double value) { return ::LobKnToMph(value); }

inline double MsToS(double value) { return ::LobMsToS(value); }
inline double UsToS(double value) { return ::LobUsToS(value); }
inline double SToMs(double value) { return ::LobSToMs(value); }
inline double SToUs(double value) { return ::LobSToUs(value); }

inline double DegCToDegF(double value) { return ::LobDegCToDegF(value); }

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
