// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#include "lobber_wizard.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

#include "lob/lob.hpp"
#include "lobber_bridge.hpp"
#include "lobber_cli.hpp"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace example {
namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

std::string Trim(const std::string& s) {
  auto start = s.begin();
  while (start != s.end() &&
         std::isspace(static_cast<unsigned char>(*start)) != 0) {
    ++start;
  }
  auto end = s.end();
  do {
    if (end == start) {
      break;
    }
    --end;
  } while (std::isspace(static_cast<unsigned char>(*end)) != 0);
  return {start, end + 1};
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
    std::cout << "  " << (i + 1) << ") " << options[i];
    if (static_cast<int>(i) + 1 == default_index) {
      std::cout << " \033[33m[Default]\033[0m";
    }
    std::cout << "\n";
  }

  while (true) {
    std::string line =
        ReadLine("Select [1-" + std::to_string(options.size()) +
                 ", enter for " + std::to_string(default_index) + "]: ");
    if (line.empty()) {
      return default_index;
    }
    char* end = nullptr;
    long val = std::strtol(line.c_str(), &end, 10);
    if (end != line.c_str() && *end == '\0' && val >= 1 &&
        val <= static_cast<long>(options.size())) {
      return static_cast<int>(val);
    }
    std::cerr << "\033[31mInvalid choice. Please enter a number between 1 and "
              << options.size() << ".\033[0m\n";
  }
}

bool PromptYesNo(const std::string& question, bool default_yes) {
  const std::string hint = default_yes ? "[Y/n]: " : "[y/N]: ";
  while (true) {
    std::string line = ReadLine(question + " " + hint);
    if (line.empty()) {
      return default_yes;
    }
    char c =
        static_cast<char>(std::tolower(static_cast<unsigned char>(line[0])));
    if (c == 'y') {
      return true;
    }
    if (c == 'n') {
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

  while (true) {
    std::string line = ReadLine(label + suffix);
    if (line.empty()) {
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
    double val = std::strtod(line.c_str(), &end);
    if (end != line.c_str() && *end == '\0' && std::isfinite(val)) {
      return val;
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
// Phase 1: Drag Source
// ---------------------------------------------------------------------------
void WizardPhaseDrag(nlohmann::json* j) {
  PrintPhaseBanner(1, 8, "Drag Source");

  const std::vector<std::string> drag_options = {
      "Single Ballistic Coefficient (BC)", "BC Velocity Bands",
      "Custom Mach vs. Cd Table", "Native Spline Coefficients (60 floats)"};
  int choice =
      PromptChoice("Select projectile drag representation:", drag_options, 1);

  if (choice == 1) {
    (*j)["DragSource"] = "bc";
    double bc = 0.0;
    while (bc <= 0.0) {
      bc = PromptDouble("Enter Ballistic Coefficient (Psi)", true);
      if (bc <= 0.0) {
        std::cerr
            << "\033[31mBallistic coefficient must be greater than 0.\033[0m\n";
      }
    }
    (*j)["BallisticCoefficientPsi"] = bc;

    const std::vector<std::string> df_options = {"G1", "G2", "G5",
                                                 "G6", "G7", "G8"};
    int df_choice =
        PromptChoice("Select reference drag function:", df_options, 1);
    const int df_map[] = {1, 2, 5, 6, 7, 8};
    (*j)["BCDragFunction"] = df_map[df_choice - 1];

    const std::vector<std::string> atmo_options = {"Army Standard Metro (ASM)",
                                                   "ICAO Reference Atmosphere"};
    int atmo_choice = PromptChoice(
        "Select BC atmosphere reference standard:", atmo_options, 1);
    (*j)["BCAtmosphere"] = (atmo_choice == 2) ? 1 : 0;
  } else if (choice == 2) {
    (*j)["DragSource"] = "bands";
    const std::vector<std::string> df_options = {"G1", "G2", "G5",
                                                 "G6", "G7", "G8"};
    int df_choice =
        PromptChoice("Select reference drag function:", df_options, 1);
    const int df_map[] = {1, 2, 5, 6, 7, 8};
    (*j)["BCDragFunction"] = df_map[df_choice - 1];

    const std::vector<std::string> atmo_options = {"Army Standard Metro (ASM)",
                                                   "ICAO Reference Atmosphere"};
    int atmo_choice = PromptChoice(
        "Select BC atmosphere reference standard:", atmo_options, 1);
    (*j)["BCAtmosphere"] = (atmo_choice == 2) ? 1 : 0;

    std::vector<BandPoint> bands;
    std::string file_path = ReadLine(
        "Load bands from file or enter interactively? (enter path or press "
        "Enter): ");
    if (!file_path.empty()) {
      std::string content = ReadFileToString(file_path);
      if (content.empty()) {
        std::cerr << "\033[31mCould not read file '" << file_path
                  << "', falling back to interactive entry.\033[0m\n";
      } else {
        bands = ParseBandsFromString(content);
        if (bands.size() < 2) {
          std::cerr << "\033[31mFile contained fewer than 2 valid band pairs, "
                       "falling back to interactive entry.\033[0m\n";
          bands.clear();
        }
      }
    }

    if (bands.empty()) {
      std::cout << "\nEnter 2 to 16 velocity (fps) and BC pairs (e.g. '3000, "
                   "0.52' or '3000 0.52').\n"
                << "Velocities must be strictly increasing. Blank line when "
                   "finished.\n";
      while (bands.size() < 16) {
        std::string line =
            ReadLine("Band " + std::to_string(bands.size() + 1) + " > ");
        if (line.empty()) {
          if (bands.size() >= 2) {
            break;
          }
          std::cerr
              << "\033[31mAt least 2 velocity bands are required.\033[0m\n";
          continue;
        }
        auto parsed = ParseBandsFromString(line);
        if (parsed.empty() || parsed[0].fps <= 0.0f || parsed[0].bc <= 0.0f) {
          std::cerr
              << "\033[31mInvalid pair. Expected: velocity_fps bc\033[0m\n";
          continue;
        }
        if (!bands.empty() && parsed[0].fps <= bands.back().fps) {
          std::cerr
              << "\033[31mVelocities must be strictly increasing (previous: "
              << bands.back().fps << " fps).\033[0m\n";
          continue;
        }
        bands.push_back(parsed[0]);
      }
    }

    nlohmann::json jbands = nlohmann::json::array();
    for (const auto& b : bands) {
      jbands.push_back({{"fps", b.fps}, {"bc", b.bc}});
    }
    (*j)["BCVelocityBands"] = jbands;
  } else if (choice == 3) {
    (*j)["DragSource"] = "table";
    std::vector<DragTablePoint> table;
    std::string file_path = ReadLine(
        "Load Mach/Cd table from file or enter interactively? (enter path or "
        "press Enter): ");
    if (!file_path.empty()) {
      std::string content = ReadFileToString(file_path);
      if (content.empty()) {
        std::cerr << "\033[31mCould not read file '" << file_path
                  << "', falling back to interactive entry.\033[0m\n";
      } else {
        table = ParseDragTableFromString(content);
        if (table.size() < 2) {
          std::cerr
              << "\033[31mFile contained fewer than 2 valid rows, falling "
                 "back to interactive entry.\033[0m\n";
          table.clear();
        }
      }
    }

    if (table.empty()) {
      std::cout << "\nEnter 2 or more Mach and Cd pairs (e.g. '2.5, 0.35' or "
                   "'2.5 0.35').\n"
                << "Mach values must be strictly increasing. Blank line when "
                   "finished.\n";
      while (true) {
        std::string line =
            ReadLine("Point " + std::to_string(table.size() + 1) + " > ");
        if (line.empty()) {
          if (table.size() >= 2) {
            break;
          }
          std::cerr << "\033[31mAt least 2 points are required.\033[0m\n";
          continue;
        }
        auto parsed = ParseDragTableFromString(line);
        if (parsed.empty() || parsed[0].mach < 0.0f || parsed[0].cd < 0.0f) {
          std::cerr << "\033[31mInvalid pair. Expected: mach cd\033[0m\n";
          continue;
        }
        if (!table.empty() && parsed[0].mach <= table.back().mach) {
          std::cerr
              << "\033[31mMach values must be strictly increasing (previous: "
              << table.back().mach << ").\033[0m\n";
          continue;
        }
        table.push_back(parsed[0]);
      }
    }

    nlohmann::json jtable = nlohmann::json::array();
    for (const auto& pt : table) {
      jtable.push_back({{"mach", pt.mach}, {"cd", pt.cd}});
    }
    (*j)["SplineFitTable"] = jtable;
  } else if (choice == 4) {
    (*j)["DragSource"] = "coefficients";
    std::vector<float> coefs;
    std::string file_path = ReadLine(
        "Load 60 spline coefficients from file or paste? (enter path or press "
        "Enter to paste): ");
    if (!file_path.empty()) {
      std::string content = ReadFileToString(file_path);
      coefs = ParseSplineCoefficientsFromString(content);
    }
    if (coefs.size() != lob::kLobCoeffsSize) {
      std::cout << "Enter or paste the 60 space/comma separated floating point "
                   "coefficients:\n";
      while (coefs.size() < lob::kLobCoeffsSize) {
        std::string line = ReadLine("> ");
        if (line.empty()) {
          break;
        }
        auto parsed = ParseSplineCoefficientsFromString(line);
        for (float v : parsed) {
          coefs.push_back(v);
          if (coefs.size() == lob::kLobCoeffsSize) {
            break;
          }
        }
        std::cout << "Read " << coefs.size() << "/" << lob::kLobCoeffsSize
                  << " coefficients.\n";
      }
    }
    (*j)["SplineCoefficients"] = coefs;
  }
}

// ---------------------------------------------------------------------------
// Phase 2: Projectile Detail & Muzzle Velocity
// ---------------------------------------------------------------------------
void WizardPhaseProjectile(nlohmann::json* j) {
  PrintPhaseBanner(2, 8, "Muzzle Velocity & Projectile Detail");

  double mv = 0.0;
  while (mv <= 0.0 || mv > 65535.0) {
    mv = PromptDouble("Enter Muzzle Velocity in feet per second (fps)", true,
                      2650.0);
    if (mv <= 0.0 || mv > 65535.0) {
      std::cerr << "\033[31mMuzzle velocity must be between 1 and 65535 "
                   "fps.\033[0m\n";
    }
  }
  (*j)["InitialVelocityFps"] = static_cast<uint16_t>(mv);

  const std::vector<std::string> tier_options = {
      "BC Only (minimal trajectory model)",
      "Mass Only (enables kinetic energy & energy cutoff)",
      "Stability Tier (mass, caliber, length, twist -> Miller stability & "
      "aerodynamic jump)",
      "Full Geometry (stability + nose, meplat, base, tail, ogive -> Boatright "
      "spin drift)"};
  int tier = PromptChoice("Select projectile detail level:", tier_options, 1);

  if (tier >= 2) {
    double mass = 0.0;
    while (mass <= 0.0) {
      mass = PromptDouble("Enter projectile mass in grains", true, 168.0);
      if (mass <= 0.0) {
        std::cerr << "\033[31mMass must be greater than 0.\033[0m\n";
      }
    }
    (*j)["MassGrains"] = mass;
  }

  if (tier >= 3) {
    double dia = 0.0;
    while (dia <= 0.0) {
      dia = PromptDouble("Enter bullet diameter (caliber) in inches", true,
                         0.308);
      if (dia <= 0.0) {
        std::cerr << "\033[31mDiameter must be greater than 0.\033[0m\n";
      }
    }
    (*j)["DiameterInch"] = dia;

    double len = 0.0;
    while (len <= 0.0) {
      len = PromptDouble("Enter bullet length in inches", true, 1.25);
      if (len <= 0.0) {
        std::cerr << "\033[31mLength must be greater than 0.\033[0m\n";
      }
    }
    (*j)["LengthInch"] = len;

    double twist = 0.0;
    while (twist <= 0.0) {
      twist = PromptDouble(
          "Enter rifling barrel twist in inches per turn (e.g. 10.0 for "
          "1:10\")",
          true, 10.0);
      if (twist <= 0.0) {
        std::cerr << "\033[31mTwist rate must be greater than 0.\033[0m\n";
      }
    }
    (*j)["TwistInchesPerTurn"] = twist;
  }

  if (tier == 4) {
    double nose = 0.0;
    while (nose <= 0.0) {
      nose = PromptDouble("Enter nose length in inches", true, 0.70);
      if (nose <= 0.0) {
        std::cerr << "\033[31mNose length must be greater than 0.\033[0m\n";
      }
    }
    (*j)["NoseLengthInch"] = nose;

    double meplat = PromptDouble("Enter meplat diameter in inches", false, 0.0);
    if (!std::isnan(meplat)) {
      (*j)["MeplatDiameterInch"] = meplat;
    }

    double base = 0.0;
    while (base <= 0.0) {
      base = PromptDouble("Enter base diameter in inches", true, 0.28);
      if (base <= 0.0) {
        std::cerr << "\033[31mBase diameter must be greater than 0.\033[0m\n";
      }
    }
    (*j)["BaseDiameterInch"] = base;

    double tail =
        PromptDouble("Enter tail (boattail) length in inches", false, 0.15);
    if (!std::isnan(tail)) {
      (*j)["TailLengthInch"] = tail;
    }

    double ogive = 0.0;
    while (ogive <= 0.0 || ogive > 1.0) {
      ogive = PromptDouble(
          "Enter ogive radius ratio (Rt/R) (greater than 0, up to 1.0)", true,
          0.5);
      if (ogive <= 0.0 || ogive > 1.0) {
        std::cerr
            << "\033[31mOgive Rt/R must be in the range (0, 1.0].\033[0m\n";
      }
    }
    (*j)["OgiveRtR"] = ogive;
  }
}

// ---------------------------------------------------------------------------
// Phase 3: Rifle Zero & Optic
// ---------------------------------------------------------------------------
void WizardPhaseZero(nlohmann::json* j) {
  PrintPhaseBanner(3, 8, "Rifle Zero & Optic");

  double optic_ht =
      PromptDouble("Enter sight/optic height above bore in inches", false, 1.5);
  if (!std::isnan(optic_ht)) {
    (*j)["OpticHeightInches"] = optic_ht;
  }

  const std::vector<std::string> zero_options = {
      "Zero by Distance (standard zero range in yards)",
      "Zero by Angle (known bore-to-sight angle in MOA)"};
  int choice = PromptChoice("Select zero method:", zero_options, 1);

  if (choice == 1) {
    double zero_dist = 0.0;
    while (zero_dist <= 0.0) {
      zero_dist = PromptDouble("Enter zero distance in yards", true, 100.0);
      if (zero_dist <= 0.0) {
        std::cerr << "\033[31mZero distance must be greater than 0.\033[0m\n";
      }
    }
    (*j)["ZeroDistanceYds"] = zero_dist;

    double impact_ht = PromptDouble(
        "Enter zero impact height in inches (+ above point of aim)", false,
        0.0);
    if (!std::isnan(impact_ht)) {
      (*j)["ZeroImpactHeightInches"] = impact_ht;
    }
  } else {
    double zero_moa =
        PromptDouble("Enter bore-to-sight zero angle in MOA", true, 0.0);
    (*j)["ZeroAngleMOA"] = zero_moa;
  }
}

// ---------------------------------------------------------------------------
// Phase 4: Atmosphere & Weather
// ---------------------------------------------------------------------------
void WizardPhaseAtmosphere(nlohmann::json* j) {
  PrintPhaseBanner(4, 8, "Atmosphere & Weather");

  const std::vector<std::string> atmo_sources = {
      "Standard Sea-Level ISA (29.92 inHg, 59°F, 0% RH)",
      "Altitude-derived atmosphere (derives pressure/temp from altitude)",
      "Measured station weather (user-entered pressure, temp, humidity)"};
  int choice =
      PromptChoice("Select weather/atmosphere model:", atmo_sources, 1);

  if (choice == 2) {
    double alt = PromptDouble("Enter firing site altitude in feet", true, 0.0);
    (*j)["AltitudeOfFiringSiteFt"] = alt;

    double rh = PromptDouble("Enter relative humidity in percent (0-100)",
                             false, kNaN, "ISA default");
    if (!std::isnan(rh)) {
      (*j)["RelativeHumidityPercent"] = rh;
    }
  } else if (choice == 3) {
    double alt = PromptDouble("Enter firing site altitude in feet", false, kNaN,
                              "sea level");
    if (!std::isnan(alt)) {
      (*j)["AltitudeOfFiringSiteFt"] = alt;
    }

    double pres = PromptDouble("Enter air pressure in inHg", false, kNaN,
                               "derive from altitude");
    if (!std::isnan(pres)) {
      (*j)["AirPressureInHg"] = pres;
    }

    double temp = PromptDouble("Enter temperature in °F", false, kNaN,
                               "derive from altitude");
    if (!std::isnan(temp)) {
      (*j)["TemperatureDegF"] = temp;
    }

    double rh = PromptDouble("Enter relative humidity in percent (0-100)",
                             false, kNaN, "ISA default");
    if (!std::isnan(rh)) {
      (*j)["RelativeHumidityPercent"] = rh;
    }

    if (!std::isnan(pres) || !std::isnan(temp)) {
      bool away = PromptYesNo(
          "Were weather readings taken at an altitude away from the firing "
          "site?",
          false);
      if (away) {
        if (!std::isnan(pres)) {
          double baro_alt = PromptDouble("Enter barometer altitude in feet",
                                         true, std::isnan(alt) ? 0.0 : alt);
          (*j)["AltitudeOfBarometerFt"] = baro_alt;
        }
        if (!std::isnan(temp)) {
          double thermo_alt = PromptDouble("Enter thermometer altitude in feet",
                                           true, std::isnan(alt) ? 0.0 : alt);
          (*j)["AltitudeOfThermometerFt"] = thermo_alt;
        }
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Phase 5: Wind Conditions
// ---------------------------------------------------------------------------
void WizardPhaseWind(nlohmann::json* j) {
  PrintPhaseBanner(5, 8, "Wind Conditions");

  const std::vector<std::string> wind_sources = {
      "Calm (no wind)", "Uniform wind (speed & clock angle / heading)",
      "Multi-station downrange wind profile"};
  int choice = PromptChoice("Select wind environment:", wind_sources, 1);

  if (choice == 2) {
    double wspd =
        PromptDouble("Enter wind speed in miles per hour (mph)", true, 10.0);
    (*j)["WindSpeedMph"] = wspd;
    if (wspd > 0.0) {
      double clock = PromptDouble(
          "Enter wind heading as clock angle (1-12, e.g. 3 for 3 o'clock "
          "crosswind)",
          true, 3.0);
      (*j)["WindHeading"] = clock;
    }
  } else if (choice == 3) {
    std::vector<lob::WindPoint> profile;
    std::string file_path = ReadLine(
        "Load wind profile from file or enter interactively? (enter path or "
        "press Enter): ");
    if (!file_path.empty()) {
      std::string content = ReadFileToString(file_path);
      if (content.empty()) {
        std::cerr << "\033[31mCould not read file '" << file_path
                  << "', falling back to interactive entry.\033[0m\n";
      } else {
        profile = ParseWindProfileFromString(content);
        if (profile.empty() || std::fabs(profile[0].range_ft) > 1e-6) {
          std::cerr
              << "\033[31mProfile must start at range 0 ft. Falling back to "
                 "interactive entry.\033[0m\n";
          profile.clear();
        }
      }
    }

    if (profile.empty()) {
      std::cout << "\nEnter 1 to 8 downrange wind stations: range_ft, "
                   "heading_deg, speed_mph [, height_ft].\n"
                << "First station must start at range 0. Ranges must be "
                   "strictly increasing. Blank line when finished.\n";
      while (profile.size() < 8) {
        std::string line =
            ReadLine("Station " + std::to_string(profile.size() + 1) + " > ");
        if (line.empty()) {
          if (!profile.empty()) {
            break;
          }
          std::cerr << "\033[31mAt least 1 station (at range 0) is "
                       "required.\033[0m\n";
          continue;
        }
        auto parsed = ParseWindProfileFromString(line);
        if (parsed.empty()) {
          std::cerr << "\033[31mInvalid station format. Expected: range_ft, "
                       "heading_deg, speed_mph [, height_ft]\033[0m\n";
          continue;
        }
        if (profile.empty() && std::fabs(parsed[0].range_ft) > 1e-6) {
          std::cerr << "\033[31mFirst station must be at range 0 ft.\033[0m\n";
          continue;
        }
        if (!profile.empty() && parsed[0].range_ft <= profile.back().range_ft) {
          std::cerr << "\033[31mStation ranges must be strictly increasing "
                       "(previous: "
                    << profile.back().range_ft << " ft).\033[0m\n";
          continue;
        }
        profile.push_back(parsed[0]);
      }
    }

    nlohmann::json jprof = nlohmann::json::array();
    bool has_height = false;
    for (const auto& p : profile) {
      nlohmann::json node = {{"range_ft", p.range_ft},
                             {"heading_deg", p.heading_deg},
                             {"speed_mph", p.speed_mph}};
      if (p.height_ft > 0.0) {
        node["height_ft"] = p.height_ft;
        has_height = true;
      }
      jprof.push_back(node);
    }
    (*j)["WindProfile"] = jprof;

    if (has_height) {
      double shear = PromptDouble(
          "Enter Hellmann wind shear exponent (0 disables height scaling)",
          false, 0.0);
      if (!std::isnan(shear)) {
        (*j)["WindShearExponent"] = shear;
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Phase 6: Shot Geometry & Coriolis
// ---------------------------------------------------------------------------
void WizardPhaseGeometry(nlohmann::json* j) {
  PrintPhaseBanner(6, 8, "Shot Geometry & Coriolis");

  bool incline = PromptYesNo(
      "Include slope / inclination angle (shooting uphill/downhill)?", false);
  if (incline) {
    double range_angle = PromptDouble(
        "Enter range angle in degrees (-89.9 to +89.9, + uphill)", true, 0.0);
    (*j)["RangeAngleDeg"] = range_angle;
  }

  bool coriolis =
      PromptYesNo("Include Earth rotation / Coriolis correction?", false);
  if (coriolis) {
    double lat = PromptDouble(
        "Enter shooting site latitude in degrees (-90 to +90, + North)", true,
        45.0);
    (*j)["LatitudeDeg"] = lat;

    double az = PromptDouble(
        "Enter azimuth / firing direction in degrees (0=North, 90=East)", true,
        90.0);
    (*j)["AzimuthDeg"] = az;
  }
}

// ---------------------------------------------------------------------------
// Phase 7: Advanced Controls & Ranges
// ---------------------------------------------------------------------------
void WizardPhaseControlsAndRanges(nlohmann::json* j) {
  PrintPhaseBanner(7, 8, "Advanced Controls & Ranges");

  bool adv = PromptYesNo(
      "Configure advanced solver controls (step size, speed/energy limits)?",
      false);
  if (adv) {
    double step = PromptDouble(
        "Enter solver step size in inches (default 36 in / 1 yd)", false, 36.0);
    if (!std::isnan(step) && step > 0.0) {
      (*j)["StepSize"] = static_cast<uint16_t>(step);
    }

    double min_spd = PromptDouble("Enter minimum speed cutoff in fps", false,
                                  kNaN, "no limit");
    if (!std::isnan(min_spd) && min_spd > 0.0) {
      (*j)["MinimumSpeed"] = static_cast<uint16_t>(min_spd);
    }

    if (j->contains("MassGrains")) {
      double min_en = PromptDouble("Enter minimum energy cutoff in ft-lbs",
                                   false, kNaN, "no limit");
      if (!std::isnan(min_en) && min_en > 0.0) {
        (*j)["MinimumEnergy"] = static_cast<uint16_t>(min_en);
      }
    }

    double max_t =
        PromptDouble("Enter maximum time of flight cutoff in seconds", false,
                     kNaN, "no limit");
    if (!std::isnan(max_t) && max_t > 0.0) {
      (*j)["MaximumTime"] = max_t;
    }
  }

  const std::vector<std::string> range_options = {
      "Standard range ladder (0 to 1000 yd by 100 yd increments)",
      "Custom span (max range and step in yards)",
      "Explicit custom range list"};
  int r_choice =
      PromptChoice("Select downrange output intervals:", range_options, 1);

  if (r_choice == 1) {
    (*j)["Ranges"] = {0, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
  } else if (r_choice == 2) {
    double max_yds = PromptDouble("Enter maximum range in yards", true, 1000.0);
    double step_yds = PromptDouble("Enter range step in yards", true, 100.0);
    if (step_yds <= 0.0) {
      step_yds = 100.0;
    }
    std::vector<double> ranges;
    for (double r = 0.0; r <= max_yds; r += step_yds) {
      ranges.push_back(r);
    }
    if (ranges.empty() || std::fabs(ranges.back() - max_yds) > 1e-6) {
      ranges.push_back(max_yds);
    }
    (*j)["Ranges"] = ranges;
  } else {
    std::cout
        << "\nEnter ranges in yards one per line (blank line when finished):\n";
    std::vector<double> ranges;
    while (true) {
      std::string line =
          ReadLine("Range " + std::to_string(ranges.size() + 1) + " (yd) > ");
      if (line.empty()) {
        break;
      }
      char* end = nullptr;
      double r = std::strtod(line.c_str(), &end);
      if (end != line.c_str() && *end == '\0' && r >= 0.0) {
        ranges.push_back(r);
      } else {
        std::cerr << "\033[31mInvalid range. Must be non-negative.\033[0m\n";
      }
    }
    if (ranges.empty()) {
      ranges = {0, 50, 100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
    }
    (*j)["Ranges"] = ranges;
  }
}

// ---------------------------------------------------------------------------
// Phase 8: Solver Configuration & Review
// ---------------------------------------------------------------------------
void WizardPhaseSolverAndReview(nlohmann::json* j) {
  PrintPhaseBanner(8, 8, "Solver Configuration & Review");

  const std::vector<std::string> path_options = {
      "Fast solver (standard numerical integration, constant atmosphere)",
      "ELR solver (lapse-scaled dynamic density along high-arc / extreme "
      "trajectories)"};
  int path_choice =
      PromptChoice("Select trajectory solver path:", path_options, 1);
  (*j)["SolverPath"] = (path_choice == 2) ? "elr" : "fast";

  const std::vector<std::string> mode_options = {
      "Forward solution (trajectory drop and windage in inches)",
      "Inverse solution (optic sight hold-off and dial adjustments)"};
  int mode_choice =
      PromptChoice("Select solution output mode:", mode_options, 1);
  (*j)["SolverMode"] = (mode_choice == 2) ? "inverse" : "forward";

  if (mode_choice == 2) {
    const std::vector<std::string> unit_options = {"MOA (Minutes of Angle)",
                                                   "MIL (Milliradians)"};
    int unit_choice =
        PromptChoice("Select preferred inverse angular unit:", unit_options, 1);
    (*j)["AngularUnit"] = (unit_choice == 2) ? "mil" : "moa";
  }

  // Pre-solve Configuration Review
  std::cout << "\n\033[1;32m=== Configuration Review ===\033[0m\n";
  std::cout << "  Drag model:       "
            << ((*j)["DragSource"].is_string()
                    ? (*j)["DragSource"].get<std::string>()
                    : "bc")
            << "\n";
  std::cout << "  Muzzle velocity:  " << (*j)["InitialVelocityFps"] << " fps\n";
  if (j->contains("MassGrains")) {
    std::cout << "  Mass:             " << (*j)["MassGrains"] << " gr\n";
  }
  if (j->contains("DiameterInch")) {
    std::cout << "  Caliber / Twist:  " << (*j)["DiameterInch"]
              << "\" / 1:" << (*j)["TwistInchesPerTurn"] << "\"\n";
  }
  if (j->contains("ZeroDistanceYds")) {
    std::cout << "  Zero:             " << (*j)["ZeroDistanceYds"] << " yd\n";
  } else if (j->contains("ZeroAngleMOA")) {
    std::cout << "  Zero angle:       " << (*j)["ZeroAngleMOA"] << " MOA\n";
  }
  if (j->contains("AltitudeOfFiringSiteFt")) {
    std::cout << "  Altitude:         " << (*j)["AltitudeOfFiringSiteFt"]
              << " ft\n";
  }
  if (j->contains("WindSpeedMph")) {
    std::cout << "  Wind:             " << (*j)["WindSpeedMph"] << " mph\n";
  } else if (j->contains("WindProfile")) {
    std::cout << "  Wind:             Profile (" << (*j)["WindProfile"].size()
              << " stations)\n";
  } else {
    std::cout << "  Wind:             Calm\n";
  }
  std::cout << "  Solver Path:      " << (*j)["SolverPath"] << "\n";
  std::cout << "  Solution Mode:    " << (*j)["SolverMode"] << "\n";
  std::cout << "\033[1;32m============================\033[0m\n\n";

  PromptYesNo("Proceed to solve trajectory?", true);
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
            << "║          Bringing parity to the CLI ballistics tool      ║\n"
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
  WizardPhaseSolverAndReview(&j);

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
