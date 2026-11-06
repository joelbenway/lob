// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "lobber_wizard.hpp"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "lob/lob.hpp"
#include "lobber_bridge.hpp"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace example {
namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr int kBaseTen = 10;
constexpr int kTotalPhases = 8;
constexpr int kPhaseDrag = 1;
constexpr int kPhaseProjectile = 2;
constexpr int kPhaseZero = 3;
constexpr int kPhaseAtmosphere = 4;
constexpr int kPhaseWind = 5;
constexpr int kPhaseGeometry = 6;
constexpr int kPhaseControlsAndRanges = 7;
constexpr int kPhaseSolverAndReview = 8;

constexpr size_t kMinPoints = 2;
constexpr size_t kMaxBands = 16;
constexpr size_t kMaxWindStations = 8;

constexpr double kDefaultG1Bc = 0.462;
constexpr double kDefaultG7Bc = 0.243;
constexpr double kDefaultOtherBc = 0.400;

constexpr double kDefaultMvFps = 2700.0;
constexpr double kMaxMvFps = 65535.0;
constexpr double kDefaultMassGrains = 168.0;
constexpr double kDefaultCaliberInch = 0.308;
constexpr double kDefaultBulletLengthInch = 1.24;
constexpr double kDefaultTwistInches = 11.0;
constexpr double kDefaultNoseLengthInch = 0.60;
constexpr double kDefaultMeplatDiameterInch = 0.05;
constexpr double kDefaultBaseDiameterInch = 0.28;
constexpr double kDefaultTailLengthInch = 0.20;
constexpr double kDefaultOgiveRatio = 0.50;

constexpr double kDefaultOpticHeightInch = 1.5;
constexpr double kDefaultZeroDistanceYards = 100.0;
constexpr double kDefaultZeroImpactHeightInch = 0.0;
constexpr double kDefaultZeroAngleMoa = 0.0;

constexpr double kDefaultWindSpeedMph = 10.0;
constexpr double kDefaultWindHeadingClock = 3.0;

constexpr double kDefaultLatitudeDeg = 45.0;
constexpr double kDefaultAzimuthDeg = 90.0;

constexpr double kDefaultStepSizeInch = 36.0;
constexpr double kDefaultMaxRangeYds = 1000.0;
constexpr double kDefaultStepRangeYds = 100.0;

constexpr double kZeroTolerance = 1e-6;

std::string Trim(const std::string& s) {
  auto start = s.begin();
  while (start != s.end() &&
         std::isspace(static_cast<unsigned char>(*start)) != 0) {
    ++start;
  }
  auto end = s.end();
  while (end != start &&
         std::isspace(static_cast<unsigned char>(*(end - 1))) != 0) {
    --end;
  }
  return {start, end};
}

std::string ReadLine(const std::string& prompt) {
  std::cout << prompt;
  std::string line;
  if (!std::getline(std::cin, line)) {
    return "";
  }
  return Trim(line);
}

void PrintPhaseBanner(int phase, int total_phases, const std::string& title) {
  std::cout << "\n\033[1;36m=== [Phase " << phase << "/" << total_phases << "] "
            << title << " ===\033[0m\n";
}

int PromptChoice(const std::string& prompt_title,
                 const std::vector<std::string>& options, int default_index) {
  if (!prompt_title.empty()) {
    std::cout << prompt_title << "\n";
  }
  for (size_t i = 0; i < options.size(); ++i) {
    std::cout << "  " << (i + 1) << ") " << options.at(i);
    if (static_cast<int>(i) + 1 == default_index) {
      std::cout << " \033[33m[Default]\033[0m";
    }
    std::cout << "\n";
  }

  while (true) {
    const std::string kPrompt = "Select [1-" + std::to_string(options.size()) +
                                ", enter for " + std::to_string(default_index) +
                                "]: ";
    const std::string kLine = ReadLine(kPrompt);
    if (kLine.empty()) {
      return default_index;
    }
    char* end = nullptr;
    const int64_t kVal = std::strtol(kLine.c_str(), &end, kBaseTen);
    if (end != kLine.c_str() && *end == '\0' && kVal >= 1 &&
        kVal <= static_cast<int64_t>(options.size())) {
      return static_cast<int>(kVal);
    }
    std::cerr << "\033[31mInvalid choice. Please enter a number between 1 and "
              << options.size() << ".\033[0m\n";
  }
}

bool PromptYesNo(const std::string& question, bool default_yes) {
  const std::string kHint = default_yes ? "[Y/n]: " : "[y/N]: ";
  while (true) {
    std::string prompt_str = question;
    prompt_str += " ";
    prompt_str += kHint;
    const std::string kLine = ReadLine(prompt_str);
    if (kLine.empty()) {
      return default_yes;
    }
    const char kC = static_cast<char>(
        std::tolower(static_cast<unsigned char>(kLine.at(0))));
    if (kC == 'y') {
      return true;
    }
    if (kC == 'n') {
      return false;
    }
    std::cerr << "\033[31mPlease enter 'y' for yes or 'n' for no.\033[0m\n";
  }
}

double PromptDouble(const std::string& label, bool required,
                    double default_val = kNaN,
                    const std::string& default_str = "") {
  std::string suffix = "> ";
  if (!default_str.empty()) {
    suffix = " \033[33m[" + default_str + "]\033[0m > ";
  } else if (!std::isnan(default_val)) {
    std::ostringstream ss;
    ss << default_val;
    suffix = " \033[33m[" + ss.str() + "]\033[0m > ";
  } else if (!required) {
    suffix = " \033[33m[blank to omit]\033[0m > ";
  }

  std::string prompt_text = label;
  prompt_text += suffix;

  while (true) {
    const std::string kLine = ReadLine(prompt_text);
    if (kLine.empty()) {
      if (!std::isnan(default_val)) {
        return default_val;
      }
      if (!required) {
        return kNaN;
      }
      std::cerr << "\033[31mThis field is required.\033[0m\n";
      continue;
    }
    char* end = nullptr;
    const double kVal = std::strtod(kLine.c_str(), &end);
    if (end != kLine.c_str() && *end == '\0' && std::isfinite(kVal)) {
      return kVal;
    }
    std::cerr << "\033[31mInvalid number. Please try again.\033[0m\n";
  }
}

std::string ReadFileToString(const std::string& path) {
  std::ifstream f(path);
  if (!f.is_open()) {
    return "";
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

// ---------------------------------------------------------------------------
// Phase 1: Drag Source Helpers & Sub-wizards
// ---------------------------------------------------------------------------
int PromptDragFunction() {
  const std::vector<std::string> kDfOptions = {"G1", "G2", "G5",
                                               "G6", "G7", "G8"};
  constexpr int kDefaultG7Choice = 5;
  const int kChoice = PromptChoice(
      "Select reference drag function:", kDfOptions, kDefaultG7Choice);
  constexpr std::array<int, 6> kDfMap = {
      static_cast<int>(lob::DragFunctionT::kG1),
      static_cast<int>(lob::DragFunctionT::kG2),
      static_cast<int>(lob::DragFunctionT::kG5),
      static_cast<int>(lob::DragFunctionT::kG6),
      static_cast<int>(lob::DragFunctionT::kG7),
      static_cast<int>(lob::DragFunctionT::kG8)};
  return kDfMap.at(static_cast<size_t>(kChoice) - 1);
}

int PromptBcAtmosphere() {
  const std::vector<std::string> kAtmoOptions = {"Army Standard Metro (ASM)",
                                                 "ICAO Reference Atmosphere"};
  const int kChoice =
      PromptChoice("Select BC atmosphere reference standard:", kAtmoOptions, 1);
  return (kChoice == 2) ? 1 : 0;
}

void WizardDragSingleBc(nlohmann::json* j) {
  (*j)["DragSource"] = "bc";
  const int kDf = PromptDragFunction();
  (*j)["BCDragFunction"] = kDf;

  double default_bc = kDefaultOtherBc;
  if (kDf == static_cast<int>(lob::DragFunctionT::kG7)) {
    default_bc = kDefaultG7Bc;
  } else if (kDf == static_cast<int>(lob::DragFunctionT::kG1)) {
    default_bc = kDefaultG1Bc;
  }

  double bc = 0.0;
  while (bc <= 0.0) {
    bc = PromptDouble("Enter Ballistic Coefficient (Psi)", /*required=*/true,
                      default_bc);
    if (bc <= 0.0) {
      std::cerr
          << "\033[31mBallistic coefficient must be greater than 0.\033[0m\n";
    }
  }
  (*j)["BallisticCoefficientPsi"] = bc;
  (*j)["BCAtmosphere"] = PromptBcAtmosphere();
}

std::vector<BandPoint> PromptInteractiveBands() {
  std::cout << "\nEnter 2 to 16 velocity (fps) and BC pairs (e.g. '3000, "
               "0.52' or '3000 0.52').\n"
            << "Velocities must be strictly increasing. Press Enter on Band 1 "
               "for sample bands.\n";
  std::vector<BandPoint> bands;
  while (bands.size() < kMaxBands) {
    const std::string kPrompt =
        "Band " + std::to_string(bands.size() + 1) + " > ";
    const std::string kLine = ReadLine(kPrompt);
    if (!std::cin) {
      break;
    }
    if (kLine.empty()) {
      if (bands.empty()) {
        constexpr float kSampleBand1Fps = 2000.0F;
        constexpr float kSampleBand1Bc = 0.235F;
        constexpr float kSampleBand2Fps = 2700.0F;
        constexpr float kSampleBand2Bc = 0.243F;
        bands = {{kSampleBand1Fps, kSampleBand1Bc},
                 {kSampleBand2Fps, kSampleBand2Bc}};
        std::cout << "Using sample velocity bands (2 stations).\n";
        break;
      }
      if (bands.size() >= kMinPoints) {
        break;
      }
      std::cerr << "\033[31mAt least 2 velocity bands are required.\033[0m\n";
      continue;
    }
    const auto kParsed = ParseBandsFromString(kLine);
    if (kParsed.empty() || kParsed.at(0).fps <= 0.0F ||
        kParsed.at(0).bc <= 0.0F) {
      std::cerr << "\033[31mInvalid pair. Expected: velocity_fps bc\033[0m\n";
      continue;
    }
    if (!bands.empty() && kParsed.at(0).fps <= bands.back().fps) {
      std::cerr << "\033[31mVelocities must be strictly increasing (previous: "
                << bands.back().fps << " fps).\033[0m\n";
      continue;
    }
    bands.push_back(kParsed.at(0));
  }
  return bands;
}

std::vector<BandPoint> LoadBandsFromFileOrInteractive() {
  const std::string kFilePath = ReadLine(
      "Load bands from file or enter interactively? (enter path or press "
      "Enter): ");
  if (!kFilePath.empty()) {
    const std::string kContent = ReadFileToString(kFilePath);
    if (kContent.empty()) {
      std::cerr << "\033[31mCould not read file '" << kFilePath
                << "', falling back to interactive entry.\033[0m\n";
    } else {
      const auto kBands = ParseBandsFromString(kContent);
      if (kBands.size() >= kMinPoints) {
        return kBands;
      }
      std::cerr << "\033[31mFile contained fewer than 2 valid band pairs, "
                   "falling back to interactive entry.\033[0m\n";
    }
  }
  return PromptInteractiveBands();
}

void WizardDragVelocityBands(nlohmann::json* j) {
  (*j)["DragSource"] = "bands";
  (*j)["BCDragFunction"] = PromptDragFunction();
  (*j)["BCAtmosphere"] = PromptBcAtmosphere();

  const auto kBands = LoadBandsFromFileOrInteractive();
  nlohmann::json jbands = nlohmann::json::array();
  for (const auto& b : kBands) {
    jbands.push_back({{"fps", b.fps}, {"bc", b.bc}});
  }
  (*j)["BCVelocityBands"] = std::move(jbands);
}

std::vector<DragTablePoint> GetSampleDragTable() {
  constexpr float kM0 = 0.0F;
  constexpr float kCd0 = 0.12F;
  constexpr float kM1 = 1.0F;
  constexpr float kCd1 = 0.25F;
  constexpr float kM2 = 2.0F;
  constexpr float kCd2 = 0.38F;
  constexpr float kM3 = 3.0F;
  constexpr float kCd3 = 0.48F;
  constexpr float kM4 = 4.0F;
  constexpr float kCd4 = 0.55F;
  constexpr float kM5 = 5.0F;
  constexpr float kCd5 = 0.60F;
  return {{kM0, kCd0}, {kM1, kCd1}, {kM2, kCd2},
          {kM3, kCd3}, {kM4, kCd4}, {kM5, kCd5}};
}

std::vector<DragTablePoint> PromptInteractiveDragTable() {
  std::cout << "\nEnter 2 or more Mach and Cd pairs (e.g. '2.5, 0.35' or "
               "'2.5 0.35').\n"
            << "Mach values must be strictly increasing. Press Enter on Point "
               "1 for sample table.\n";
  std::vector<DragTablePoint> table;
  while (true) {
    const std::string kPrompt =
        "Point " + std::to_string(table.size() + 1) + " > ";
    const std::string kLine = ReadLine(kPrompt);
    if (!std::cin) {
      break;
    }
    if (kLine.empty()) {
      if (table.empty()) {
        table = GetSampleDragTable();
        std::cout << "Using sample Mach/Cd table (6 points).\n";
        break;
      }
      if (table.size() >= kMinPoints) {
        break;
      }
      std::cerr << "\033[31mAt least 2 points are required.\033[0m\n";
      continue;
    }
    const auto kParsed = ParseDragTableFromString(kLine);
    if (kParsed.empty() || kParsed.at(0).mach < 0.0F ||
        kParsed.at(0).cd < 0.0F) {
      std::cerr << "\033[31mInvalid pair. Expected: mach cd\033[0m\n";
      continue;
    }
    if (!table.empty() && kParsed.at(0).mach <= table.back().mach) {
      std::cerr << "\033[31mMach values must be strictly increasing (previous: "
                << table.back().mach << ").\033[0m\n";
      continue;
    }
    table.push_back(kParsed.at(0));
  }
  return table;
}

std::vector<DragTablePoint> LoadDragTableFromFileOrInteractive() {
  const std::string kFilePath = ReadLine(
      "Load Mach/Cd table from file or enter interactively? (enter path or "
      "press Enter): ");
  if (!kFilePath.empty()) {
    const std::string kContent = ReadFileToString(kFilePath);
    if (kContent.empty()) {
      std::cerr << "\033[31mCould not read file '" << kFilePath
                << "', falling back to interactive entry.\033[0m\n";
    } else {
      const auto kTable = ParseDragTableFromString(kContent);
      if (kTable.size() >= kMinPoints) {
        return kTable;
      }
      std::cerr << "\033[31mFile contained fewer than 2 valid rows, falling "
                   "back to interactive entry.\033[0m\n";
    }
  }
  return PromptInteractiveDragTable();
}

void WizardDragCustomTable(nlohmann::json* j) {
  (*j)["DragSource"] = "table";
  const auto kTable = LoadDragTableFromFileOrInteractive();
  nlohmann::json jtable = nlohmann::json::array();
  for (const auto& pt : kTable) {
    jtable.push_back({{"mach", pt.mach}, {"cd", pt.cd}});
  }
  (*j)["SplineFitTable"] = std::move(jtable);
}

std::vector<float> PromptInteractiveSplineCoefficients() {
  std::cout << "Enter or paste the 60 space/comma separated floating point "
               "coefficients (press Enter for sample coefficients):\n";
  std::vector<float> coefs;
  while (coefs.size() < lob::kLobCoeffsSize) {
    const std::string kLine = ReadLine("> ");
    if (!std::cin) {
      break;
    }
    if (kLine.empty()) {
      if (coefs.empty()) {
        const auto kSampleCtx =
            lob::Builder()
                .BallisticCoefficientPsi(kDefaultG7Bc)
                .BCDragFunction(lob::DragFunctionT::kG7)
                .InitialVelocityFps(static_cast<uint16_t>(kDefaultMvFps))
                .ZeroAngleMOA(kDefaultZeroAngleMoa)
                .Build();
        coefs.assign(kSampleCtx.drags.begin(), kSampleCtx.drags.end());
        std::cout << "Using sample spline coefficients (60 floats).\n";
        break;
      }
      continue;
    }
    const auto kParsed = ParseSplineCoefficientsFromString(kLine);
    for (const float kVal : kParsed) {
      coefs.push_back(kVal);
      if (coefs.size() == lob::kLobCoeffsSize) {
        break;
      }
    }
    std::cout << "Read " << coefs.size() << "/" << lob::kLobCoeffsSize
              << " coefficients.\n";
  }
  return coefs;
}

std::vector<float> LoadSplineCoefficientsFromFileOrInteractive() {
  const std::string kFilePath = ReadLine(
      "Load 60 spline coefficients from file or paste? (enter path or press "
      "Enter to paste): ");
  if (!kFilePath.empty()) {
    const std::string kContent = ReadFileToString(kFilePath);
    auto coefs = ParseSplineCoefficientsFromString(kContent);
    if (coefs.size() == lob::kLobCoeffsSize) {
      return coefs;
    }
  }
  return PromptInteractiveSplineCoefficients();
}

void WizardDragSplineCoefficients(nlohmann::json* j) {
  (*j)["DragSource"] = "coefficients";
  const auto kCoefs = LoadSplineCoefficientsFromFileOrInteractive();
  (*j)["SplineCoefficients"] = kCoefs;
}

void WizardPhaseDrag(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseDrag, kTotalPhases, "Drag Source");

  const std::vector<std::string> kDragOptions = {
      "Single Ballistic Coefficient (BC)", "BC Velocity Bands",
      "Custom Mach vs. Cd Table", "Native Spline Coefficients (60 floats)"};
  const int kChoice =
      PromptChoice("Select projectile drag representation:", kDragOptions, 1);

  switch (kChoice) {
    case 1:
      WizardDragSingleBc(j);
      break;
    case 2:
      WizardDragVelocityBands(j);
      break;
    case 3:
      WizardDragCustomTable(j);
      break;
    case 4:
      WizardDragSplineCoefficients(j);
      break;
    default:
      WizardDragSingleBc(j);
      break;
  }
}

// ---------------------------------------------------------------------------
// Phase 2: Projectile Detail & Muzzle Velocity
// ---------------------------------------------------------------------------
void PromptProjectileMass(nlohmann::json* j) {
  double mass = 0.0;
  while (mass <= 0.0) {
    mass = PromptDouble("Enter projectile mass in grains", /*required=*/true,
                        kDefaultMassGrains);
    if (mass <= 0.0) {
      std::cerr << "\033[31mMass must be greater than 0.\033[0m\n";
    }
  }
  (*j)["MassGrains"] = mass;
}

void PromptProjectileDimensions(nlohmann::json* j) {
  double dia = 0.0;
  while (dia <= 0.0) {
    dia = PromptDouble("Enter bullet diameter (caliber) in inches",
                       /*required=*/true, kDefaultCaliberInch);
    if (dia <= 0.0) {
      std::cerr << "\033[31mDiameter must be greater than 0.\033[0m\n";
    }
  }
  (*j)["DiameterInch"] = dia;

  double len = 0.0;
  while (len <= 0.0) {
    len = PromptDouble("Enter bullet length in inches", /*required=*/true,
                       kDefaultBulletLengthInch);
    if (len <= 0.0) {
      std::cerr << "\033[31mLength must be greater than 0.\033[0m\n";
    }
  }
  (*j)["LengthInch"] = len;

  double twist = 0.0;
  while (twist <= 0.0) {
    twist = PromptDouble(
        "Enter rifling barrel twist in inches per turn (e.g. 11.0 for "
        "1:11\")",
        /*required=*/true, kDefaultTwistInches);
    if (twist <= 0.0) {
      std::cerr << "\033[31mTwist rate must be greater than 0.\033[0m\n";
    }
  }
  (*j)["TwistInchesPerTurn"] = twist;
}

void PromptProjectileGeometry(nlohmann::json* j) {
  double nose = 0.0;
  while (nose <= 0.0) {
    nose = PromptDouble("Enter nose length in inches", /*required=*/true,
                        kDefaultNoseLengthInch);
    if (nose <= 0.0) {
      std::cerr << "\033[31mNose length must be greater than 0.\033[0m\n";
    }
  }
  (*j)["NoseLengthInch"] = nose;

  const double kMeplat =
      PromptDouble("Enter meplat diameter in inches", /*required=*/false,
                   kDefaultMeplatDiameterInch);
  if (!std::isnan(kMeplat)) {
    (*j)["MeplatDiameterInch"] = kMeplat;
  }

  double base = 0.0;
  while (base <= 0.0) {
    base = PromptDouble("Enter base diameter in inches", /*required=*/true,
                        kDefaultBaseDiameterInch);
    if (base <= 0.0) {
      std::cerr << "\033[31mBase diameter must be greater than 0.\033[0m\n";
    }
  }
  (*j)["BaseDiameterInch"] = base;

  const double kTail = PromptDouble("Enter tail (boattail) length in inches",
                                    /*required=*/false, kDefaultTailLengthInch);
  if (!std::isnan(kTail)) {
    (*j)["TailLengthInch"] = kTail;
  }

  double ogive = 0.0;
  while (ogive <= 0.0 || ogive > 1.0) {
    ogive = PromptDouble(
        "Enter ogive radius ratio (Rt/R) (greater than 0, up to 1.0)",
        /*required=*/true, kDefaultOgiveRatio);
    if (ogive <= 0.0 || ogive > 1.0) {
      std::cerr << "\033[31mOgive Rt/R must be in the range (0, 1.0].\033[0m\n";
    }
  }
  (*j)["OgiveRtR"] = ogive;
}

void WizardPhaseProjectile(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseProjectile, kTotalPhases,
                   "Muzzle Velocity & Projectile Detail");

  double mv = 0.0;
  while (mv <= 0.0 || mv > kMaxMvFps) {
    mv = PromptDouble("Enter Muzzle Velocity in feet per second (fps)",
                      /*required=*/true, kDefaultMvFps);
    if (mv <= 0.0 || mv > kMaxMvFps) {
      std::cerr << "\033[31mMuzzle velocity must be between 1 and 65535 "
                   "fps.\033[0m\n";
    }
  }
  (*j)["InitialVelocityFps"] = static_cast<uint16_t>(mv);

  const std::vector<std::string> kTierOptions = {
      "BC Only (minimal trajectory model)",
      "Mass Only (enables kinetic energy & energy cutoff)",
      "Stability Tier (mass, caliber, length, twist -> Miller stability & "
      "aerodynamic jump)",
      "Full Geometry (stability + nose, meplat, base, tail, ogive -> Boatright "
      "spin drift)"};
  const int kTier =
      PromptChoice("Select projectile detail level:", kTierOptions, 2);

  if (kTier >= 2) {
    PromptProjectileMass(j);
  }
  if (kTier >= 3) {
    PromptProjectileDimensions(j);
  }
  if (kTier >= 4) {
    PromptProjectileGeometry(j);
  }
}

// ---------------------------------------------------------------------------
// Phase 3: Rifle Zero & Optic
// ---------------------------------------------------------------------------
void WizardPhaseZero(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseZero, kTotalPhases, "Rifle Zero & Optic");

  const double kOpticHt =
      PromptDouble("Enter sight/optic height above bore in inches",
                   /*required=*/false, kDefaultOpticHeightInch);
  if (!std::isnan(kOpticHt)) {
    (*j)["OpticHeightInches"] = kOpticHt;
  }

  const std::vector<std::string> kZeroOptions = {
      "Zero by Distance (standard zero range in yards)",
      "Zero by Angle (known bore-to-sight angle in MOA)"};
  const int kChoice = PromptChoice("Select zero method:", kZeroOptions, 1);

  if (kChoice == 1) {
    double zero_dist = 0.0;
    while (zero_dist <= 0.0) {
      zero_dist = PromptDouble("Enter zero distance in yards",
                               /*required=*/true, kDefaultZeroDistanceYards);
      if (zero_dist <= 0.0) {
        std::cerr << "\033[31mZero distance must be greater than 0.\033[0m\n";
      }
    }
    (*j)["ZeroDistanceYds"] = zero_dist;

    const double kImpactHt = PromptDouble(
        "Enter zero impact height in inches (+ above point of aim)",
        /*required=*/false, kDefaultZeroImpactHeightInch);
    if (!std::isnan(kImpactHt)) {
      (*j)["ZeroImpactHeightInches"] = kImpactHt;
    }
  } else {
    const double kZeroMoa =
        PromptDouble("Enter bore-to-sight zero angle in MOA",
                     /*required=*/true, kDefaultZeroAngleMoa);
    (*j)["ZeroAngleMOA"] = kZeroMoa;
  }
}

// ---------------------------------------------------------------------------
// Phase 4: Atmosphere & Weather
// ---------------------------------------------------------------------------
void PromptMeasuredAtmosphere(nlohmann::json* j) {
  const double kAlt = PromptDouble("Enter firing site altitude in feet",
                                   /*required=*/false, kNaN, "sea level");
  if (!std::isnan(kAlt)) {
    (*j)["AltitudeOfFiringSiteFt"] = kAlt;
  }

  const double kPres =
      PromptDouble("Enter air pressure in inHg",
                   /*required=*/false, kNaN, "derive from altitude");
  if (!std::isnan(kPres)) {
    (*j)["AirPressureInHg"] = kPres;
  }

  const double kTemp =
      PromptDouble("Enter temperature in °F",
                   /*required=*/false, kNaN, "derive from altitude");
  if (!std::isnan(kTemp)) {
    (*j)["TemperatureDegF"] = kTemp;
  }

  const double kRh = PromptDouble("Enter relative humidity in percent (0-100)",
                                  /*required=*/false, kNaN, "ISA default");
  if (!std::isnan(kRh)) {
    (*j)["RelativeHumidityPercent"] = kRh;
  }

  if (!std::isnan(kPres) || !std::isnan(kTemp)) {
    const bool kAway = PromptYesNo(
        "Were weather readings taken at an altitude away from the firing "
        "site?",
        /*default_yes=*/false);
    if (kAway) {
      const double kDefaultAlt = std::isnan(kAlt) ? 0.0 : kAlt;
      if (!std::isnan(kPres)) {
        const double kBaroAlt = PromptDouble("Enter barometer altitude in feet",
                                             /*required=*/true, kDefaultAlt);
        (*j)["AltitudeOfBarometerFt"] = kBaroAlt;
      }
      if (!std::isnan(kTemp)) {
        const double kThermoAlt =
            PromptDouble("Enter thermometer altitude in feet",
                         /*required=*/true, kDefaultAlt);
        (*j)["AltitudeOfThermometerFt"] = kThermoAlt;
      }
    }
  }
}

void WizardPhaseAtmosphere(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseAtmosphere, kTotalPhases, "Atmosphere & Weather");

  const std::vector<std::string> kAtmoSources = {
      "Standard Sea-Level ISA (29.92 inHg, 59°F, 0% RH)",
      "Altitude-derived atmosphere (derives pressure/temp from altitude)",
      "Measured station weather (user-entered pressure, temp, humidity)"};
  const int kChoice =
      PromptChoice("Select weather/atmosphere model:", kAtmoSources, 1);

  if (kChoice == 2) {
    const double kAlt = PromptDouble("Enter firing site altitude in feet",
                                     /*required=*/true, 0.0);
    (*j)["AltitudeOfFiringSiteFt"] = kAlt;

    const double kRh =
        PromptDouble("Enter relative humidity in percent (0-100)",
                     /*required=*/false, kNaN, "ISA default");
    if (!std::isnan(kRh)) {
      (*j)["RelativeHumidityPercent"] = kRh;
    }
  } else if (kChoice == 3) {
    PromptMeasuredAtmosphere(j);
  }
}

// ---------------------------------------------------------------------------
// Phase 5: Wind Conditions
// ---------------------------------------------------------------------------
std::vector<lob::WindPoint> PromptInteractiveWindProfile() {
  std::cout << "\nEnter 1 to 8 downrange wind stations: range_ft, "
               "heading_deg, speed_mph [, height_ft].\n"
            << "First station must start at range 0. Ranges must be "
               "strictly increasing. Press Enter on Station 1 for sample "
               "profile.\n";
  std::vector<lob::WindPoint> profile;
  while (profile.size() < kMaxWindStations) {
    const std::string kPrompt =
        "Station " + std::to_string(profile.size() + 1) + " > ";
    const std::string kLine = ReadLine(kPrompt);
    if (kLine.empty()) {
      if (profile.empty()) {
        constexpr double kSampleRange1 = 0.0;
        constexpr double kSampleHeading = 90.0;
        constexpr double kSampleSpeed1 = 10.0;
        constexpr double kSampleHeight = 6.0;
        constexpr double kSampleRange2 = 1500.0;
        constexpr double kSampleSpeed2 = 12.0;
        profile = {
            {kSampleRange1, kSampleHeading, kSampleSpeed1, kSampleHeight},
            {kSampleRange2, kSampleHeading, kSampleSpeed2, kSampleHeight}};
        std::cout << "Using sample wind profile (2 stations).\n";
        break;
      }
      break;
    }
    const auto kParsed = ParseWindProfileFromString(kLine);
    if (kParsed.empty()) {
      std::cerr << "\033[31mInvalid station format. Expected: range_ft, "
                   "heading_deg, speed_mph [, height_ft]\033[0m\n";
      continue;
    }
    if (profile.empty() && std::fabs(kParsed.at(0).range_ft) > kZeroTolerance) {
      std::cerr << "\033[31mFirst station must be at range 0 ft.\033[0m\n";
      continue;
    }
    if (!profile.empty() && kParsed.at(0).range_ft <= profile.back().range_ft) {
      std::cerr << "\033[31mStation ranges must be strictly increasing "
                   "(previous: "
                << profile.back().range_ft << " ft).\033[0m\n";
      continue;
    }
    profile.push_back(kParsed.at(0));
  }
  return profile;
}

bool IsValidWindProfile(const std::vector<lob::WindPoint>& profile) {
  if (profile.empty() || profile.size() > kMaxWindStations) {
    return false;
  }
  if (std::fabs(profile.front().range_ft) > kZeroTolerance) {
    return false;
  }
  for (size_t i = 1; i < profile.size(); ++i) {
    if (profile.at(i).range_ft <= profile.at(i - 1).range_ft) {
      return false;
    }
  }
  return true;
}

std::vector<lob::WindPoint> LoadWindProfileFromFileOrInteractive() {
  const std::string kFilePath = ReadLine(
      "Load wind profile from file or enter interactively? (enter path or "
      "press Enter): ");
  if (!kFilePath.empty()) {
    const std::string kContent = ReadFileToString(kFilePath);
    if (kContent.empty()) {
      std::cerr << "\033[31mCould not read file '" << kFilePath
                << "', falling back to interactive entry.\033[0m\n";
    } else {
      const auto kProfile = ParseWindProfileFromString(kContent);
      if (IsValidWindProfile(kProfile)) {
        return kProfile;
      }
      std::cerr
          << "\033[31mFile contained invalid wind profile, falling back to "
             "interactive entry.\033[0m\n";
    }
  }
  return PromptInteractiveWindProfile();
}

void PromptMultiStationWind(nlohmann::json* j) {
  const auto kProfile = LoadWindProfileFromFileOrInteractive();
  nlohmann::json jprof = nlohmann::json::array();
  bool has_height = false;
  for (const auto& p : kProfile) {
    nlohmann::json node = {{"range_ft", p.range_ft},
                           {"heading_deg", p.heading_deg},
                           {"speed_mph", p.speed_mph}};
    if (p.height_ft > 0.0) {
      node["height_ft"] = p.height_ft;
      has_height = true;
    }
    jprof.push_back(node);
  }
  (*j)["WindProfile"] = std::move(jprof);

  if (has_height) {
    const double kShear = PromptDouble(
        "Enter Hellmann wind shear exponent (0 disables height scaling)",
        /*required=*/false, 0.0);
    if (!std::isnan(kShear)) {
      (*j)["WindShearExponent"] = kShear;
    }
  }
}

void WizardPhaseWind(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseWind, kTotalPhases, "Wind Conditions");

  const std::vector<std::string> kWindSources = {
      "Calm (no wind)", "Uniform wind (speed & clock angle / heading)",
      "Multi-station downrange wind profile"};
  const int kChoice = PromptChoice("Select wind environment:", kWindSources, 2);

  if (kChoice == 2) {
    const double kWspd =
        PromptDouble("Enter wind speed in miles per hour (mph)",
                     /*required=*/true, kDefaultWindSpeedMph);
    (*j)["WindSpeedMph"] = kWspd;
    if (kWspd > 0.0) {
      const double kClock = PromptDouble(
          "Enter wind heading as clock angle (1-12, e.g. 3 for 3 o'clock "
          "crosswind)",
          /*required=*/true, kDefaultWindHeadingClock);
      (*j)["WindHeading"] = kClock;
    }
  } else if (kChoice == 3) {
    PromptMultiStationWind(j);
  }
}

// ---------------------------------------------------------------------------
// Phase 6: Shot Geometry & Coriolis
// ---------------------------------------------------------------------------
void WizardPhaseGeometry(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseGeometry, kTotalPhases, "Shot Geometry & Coriolis");

  const bool kIncline = PromptYesNo(
      "Include slope / inclination angle (shooting uphill/downhill)?",
      /*default_yes=*/false);
  if (kIncline) {
    const double kRangeAngle =
        PromptDouble("Enter range angle in degrees (-89.9 to +89.9, + uphill)",
                     /*required=*/true, 0.0);
    (*j)["RangeAngleDeg"] = kRangeAngle;
  }

  const bool kCoriolis = PromptYesNo(
      "Include Earth rotation / Coriolis correction?", /*default_yes=*/false);
  if (kCoriolis) {
    const double kLat = PromptDouble(
        "Enter shooting site latitude in degrees (-90 to +90, + North)",
        /*required=*/true, kDefaultLatitudeDeg);
    (*j)["LatitudeDeg"] = kLat;

    const double kAz = PromptDouble(
        "Enter azimuth / firing direction in degrees (0=North, 90=East)",
        /*required=*/true, kDefaultAzimuthDeg);
    (*j)["AzimuthDeg"] = kAz;
  }
}

// ---------------------------------------------------------------------------
// Phase 7: Advanced Controls & Ranges
// ---------------------------------------------------------------------------
void PromptAdvancedControls(nlohmann::json* j) {
  constexpr double kMaxU16 = 65535.0;
  while (true) {
    const double kStep =
        PromptDouble("Enter solver step size in inches (default 36 in / 1 yd)",
                     /*required=*/false, kDefaultStepSizeInch);
    if (std::isnan(kStep)) {
      break;
    }
    if (kStep >= 1.0 && kStep <= kMaxU16) {
      (*j)["StepSize"] = static_cast<uint16_t>(kStep);
      break;
    }
    std::cerr
        << "\033[31mStep size must be between 1 and 65535 inches.\033[0m\n";
  }

  while (true) {
    const double kMinSpd = PromptDouble("Enter minimum speed cutoff in fps",
                                        /*required=*/false, kNaN, "no limit");
    if (std::isnan(kMinSpd)) {
      break;
    }
    if (kMinSpd >= 1.0 && kMinSpd <= kMaxU16) {
      (*j)["MinimumSpeed"] = static_cast<uint16_t>(kMinSpd);
      break;
    }
    std::cerr
        << "\033[31mMinimum speed must be between 1 and 65535 fps.\033[0m\n";
  }

  if (j->contains("MassGrains")) {
    while (true) {
      const double kMinEn =
          PromptDouble("Enter minimum energy cutoff in ft-lbs",
                       /*required=*/false, kNaN, "no limit");
      if (std::isnan(kMinEn)) {
        break;
      }
      if (kMinEn >= 1.0 && kMinEn <= kMaxU16) {
        (*j)["MinimumEnergy"] = static_cast<uint16_t>(kMinEn);
        break;
      }
      std::cerr << "\033[31mMinimum energy must be between 1 and 65535 "
                   "ft-lbs.\033[0m\n";
    }
  }

  const double kMaxT =
      PromptDouble("Enter maximum time of flight cutoff in seconds",
                   /*required=*/false, kNaN, "no limit");
  if (!std::isnan(kMaxT) && kMaxT > 0.0) {
    (*j)["MaximumTime"] = kMaxT;
  }
}

std::vector<uint32_t> BuildDefaultLadder() {
  constexpr uint32_t kFirstStep = 50;
  constexpr uint32_t kStep = 100;
  constexpr uint32_t kMax = 1000;
  std::vector<uint32_t> ladder;
  constexpr size_t kDefaultLadderCount = 12;
  ladder.reserve(kDefaultLadderCount);
  ladder.push_back(0);
  ladder.push_back(kFirstStep);
  for (uint32_t range_val = kStep; range_val <= kMax; range_val += kStep) {
    ladder.push_back(range_val);
  }
  return ladder;
}

std::vector<double> BuildSpanRanges() {
  constexpr size_t kMaxSpanSteps = 10000;
  const double kMaxYds = PromptDouble("Enter maximum range in yards",
                                      /*required=*/true, kDefaultMaxRangeYds);
  if (!std::isfinite(kMaxYds) || kMaxYds <= 0.0) {
    std::cerr << "\033[31mMaximum range must be greater than 0.\033[0m\n";
    return {};
  }
  double step_yds = PromptDouble("Enter range step in yards", /*required=*/true,
                                 kDefaultStepRangeYds);
  if (!std::isfinite(step_yds) || step_yds <= 0.0) {
    step_yds = kDefaultStepRangeYds;
  }
  const double kStepsVal = std::floor(kMaxYds / step_yds);
  if (!std::isfinite(kStepsVal) || kStepsVal < 0.0 ||
      kStepsVal > static_cast<double>(kMaxSpanSteps)) {
    std::cerr << "\033[31mToo many range steps (maximum 10000). Increase the "
                 "step or reduce the maximum range.\033[0m\n";
    return {};
  }
  const auto kSteps = static_cast<size_t>(kStepsVal);
  std::vector<double> ranges;
  ranges.reserve(kSteps + 2);
  for (size_t i = 0; i <= kSteps; ++i) {
    ranges.push_back(static_cast<double>(i) * step_yds);
  }
  if (ranges.empty() || std::fabs(ranges.back() - kMaxYds) > kZeroTolerance) {
    ranges.push_back(kMaxYds);
  }
  return ranges;
}

std::vector<double> PromptCustomRanges() {
  std::cout
      << "\nEnter ranges in yards one per line (blank line when finished):\n";
  std::vector<double> ranges;
  while (true) {
    const std::string kPrompt =
        "Range " + std::to_string(ranges.size() + 1) + " (yd) > ";
    const std::string kLine = ReadLine(kPrompt);
    if (kLine.empty()) {
      break;
    }
    char* end = nullptr;
    const double kR = std::strtod(kLine.c_str(), &end);
    if (end != kLine.c_str() && *end == '\0' && kR >= 0.0) {
      ranges.push_back(kR);
    } else {
      std::cerr << "\033[31mInvalid range. Must be non-negative.\033[0m\n";
    }
  }
  if (ranges.empty()) {
    const auto kDefaultLadder = BuildDefaultLadder();
    ranges.assign(kDefaultLadder.begin(), kDefaultLadder.end());
  }
  return ranges;
}

void WizardPhaseControlsAndRanges(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseControlsAndRanges, kTotalPhases,
                   "Advanced Controls & Ranges");

  const bool kAdv = PromptYesNo(
      "Configure advanced solver controls (step size, speed/energy limits)?",
      /*default_yes=*/false);
  if (kAdv) {
    PromptAdvancedControls(j);
  }

  const std::vector<std::string> kRangeOptions = {
      "Standard range ladder (0 to 1000 yd by 100 yd increments)",
      "Custom span (max range and step in yards)",
      "Explicit custom range list"};
  const int kRChoice =
      PromptChoice("Select downrange output intervals:", kRangeOptions, 1);

  if (kRChoice == 1) {
    (*j)["Ranges"] = BuildDefaultLadder();
  } else if (kRChoice == 2) {
    std::vector<double> ranges;
    while (ranges.empty() && std::cin) {
      ranges = BuildSpanRanges();
    }
    if (ranges.empty()) {
      const auto kDefaultLadder = BuildDefaultLadder();
      ranges.assign(kDefaultLadder.begin(), kDefaultLadder.end());
    }
    (*j)["Ranges"] = std::move(ranges);
  } else {
    (*j)["Ranges"] = PromptCustomRanges();
  }
}

// ---------------------------------------------------------------------------
// Phase 8: Solver Configuration & Review
// ---------------------------------------------------------------------------
void PrintConfigurationReview(const nlohmann::json& j) {
  std::cout << "\n\033[1;32m=== Configuration Review ===\033[0m\n";
  std::cout << "  Drag model:       "
            << (j["DragSource"].is_string() ? j["DragSource"].get<std::string>()
                                            : "bc")
            << "\n";
  std::cout << "  Muzzle velocity:  " << j["InitialVelocityFps"] << " fps\n";
  if (j.contains("MassGrains")) {
    std::cout << "  Mass:             " << j["MassGrains"] << " gr\n";
  }
  if (j.contains("DiameterInch")) {
    std::cout << "  Caliber / Twist:  " << j["DiameterInch"]
              << "\" / 1:" << j["TwistInchesPerTurn"] << "\"\n";
  }
  if (j.contains("ZeroDistanceYds")) {
    std::cout << "  Zero:             " << j["ZeroDistanceYds"] << " yd\n";
  } else if (j.contains("ZeroAngleMOA")) {
    std::cout << "  Zero angle:       " << j["ZeroAngleMOA"] << " MOA\n";
  }
  if (j.contains("AltitudeOfFiringSiteFt")) {
    std::cout << "  Altitude:         " << j["AltitudeOfFiringSiteFt"]
              << " ft\n";
  }
  if (j.contains("WindSpeedMph")) {
    std::cout << "  Wind:             " << j["WindSpeedMph"] << " mph\n";
  } else if (j.contains("WindProfile")) {
    std::cout << "  Wind:             Profile (" << j["WindProfile"].size()
              << " stations)\n";
  } else {
    std::cout << "  Wind:             Calm\n";
  }
  std::cout << "  Solver Path:      " << j["SolverPath"] << "\n";
  std::cout << "  Solution Mode:    " << j["SolverMode"] << "\n";
  std::cout << "\033[1;32m============================\033[0m\n\n";
}

bool WizardPhaseSolverAndReview(nlohmann::json* j) {
  PrintPhaseBanner(kPhaseSolverAndReview, kTotalPhases,
                   "Solver Configuration & Review");

  const std::vector<std::string> kPathOptions = {
      "Fast solver (standard numerical integration, constant atmosphere)",
      "ELR solver (lapse-scaled dynamic density along high-arc / extreme "
      "trajectories)"};
  const int kPathChoice =
      PromptChoice("Select trajectory solver path:", kPathOptions, 1);
  (*j)["SolverPath"] = (kPathChoice == 2) ? "elr" : "fast";

  const std::vector<std::string> kModeOptions = {
      "Forward solution (trajectory drop and windage in inches)",
      "Inverse solution (optic sight hold-off and dial adjustments)"};
  const int kModeChoice =
      PromptChoice("Select solution output mode:", kModeOptions, 1);
  (*j)["SolverMode"] = (kModeChoice == 2) ? "inverse" : "forward";

  if (kModeChoice == 2) {
    const std::vector<std::string> kUnitOptions = {"MOA (Minutes of Angle)",
                                                   "MIL (Milliradians)"};
    const int kUnitChoice =
        PromptChoice("Select preferred inverse angular unit:", kUnitOptions, 1);
    (*j)["AngularUnit"] = (kUnitChoice == 2) ? "mil" : "moa";
  }

  PrintConfigurationReview(*j);

  if (!PromptYesNo("Proceed to solve trajectory?", /*default_yes=*/true)) {
    std::cout << "Trajectory solve canceled.\n";
    return false;
  }
  return true;
}

}  // namespace

bool IsInteractive() {
#ifdef _WIN32
  return _isatty(_fileno(stdin)) != 0;
#else
  return isatty(STDIN_FILENO) != 0;
#endif
}

void PrintGreeting() {
  std::cout << "\033[1;34m"
            << "╔══════════════════════════════════════════════════════════╗\n"
            << "║                 LOBBER BALLISTICS WIZARD                 ║\n"
            << "╚══════════════════════════════════════════════════════════╝\n"
            << "\033[0m"
            << "Welcome to lobber! Follow the 8 phases to set up your load.\n"
            << "Press Enter on any field to accept defaults or skip optional "
               "values.\n";
}

nlohmann::json RunWizard() {
  nlohmann::json j;
  PrintGreeting();

  WizardPhaseDrag(&j);
  WizardPhaseProjectile(&j);
  WizardPhaseZero(&j);
  WizardPhaseAtmosphere(&j);
  WizardPhaseWind(&j);
  WizardPhaseGeometry(&j);
  WizardPhaseControlsAndRanges(&j);
  if (!WizardPhaseSolverAndReview(&j)) {
    return {};
  }

  return j;
}

}  // namespace example

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
