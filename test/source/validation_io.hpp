// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

// Test-only artifact writer (Phase 1). Hand-rolled: the project ships zero
// dependencies, and only the writer side is needed until Phase 3 (which adds
// the reference-data reader). C++14, no exceptions from this header itself.

#pragma once

#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace tests {

inline std::string JsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (const char kCh : s) {
    if (kCh == '"' || kCh == '\\') {
      out += '\\';
    }
    out += kCh;
  }
  return out;
}

// NaN/non-finite -> null (CSV uses empty field instead, see ToCsv).
inline std::string JsonDouble(double v) {
  constexpr int kJsonPrecisionDigits = 10;
  if (!std::isfinite(v)) {
    return "null";
  }
  std::ostringstream os;
  os << std::setprecision(kJsonPrecisionDigits) << v;
  return os.str();
}

struct ArtifactRung {
  uint32_t step_in = 0;
  double elevation_in = 0.0;
  double elevation_delta_in = 0.0;
};

struct ConvergenceArtifact {
  std::string provenance_lob_version;
  std::string provenance_git_sha;
  std::string solver_config;
  std::vector<ArtifactRung> rungs;

  void AddRung(uint32_t step_in, double elevation_in,
               double elevation_delta_in) {
    ArtifactRung rung;
    rung.step_in = step_in;
    rung.elevation_in = elevation_in;
    rung.elevation_delta_in = elevation_delta_in;
    rungs.push_back(rung);
  }

  std::string ToJson() const {
    std::ostringstream os;
    os << R"({"provenance":{"lob_version":")"
       << JsonEscape(provenance_lob_version) << R"(","git_sha":")"
       << JsonEscape(provenance_git_sha) << R"("},"solver_config":")"
       << JsonEscape(solver_config) << R"(","rungs":[)";
    bool first = true;
    for (const auto& rung : rungs) {
      if (!first) {
        os << ",";
      }
      first = false;
      os << R"({"step_in":)" << rung.step_in << R"(,"elevation_in":)"
         << JsonDouble(rung.elevation_in) << R"(,"elevation_delta_in":)"
         << JsonDouble(rung.elevation_delta_in) << "}";
    }
    os << "]}";
    return os.str();
  }

  std::string ToCsv() const {
    std::ostringstream os;
    os << "step_in,elevation_in,elevation_delta_in\n";
    for (const auto& rung : rungs) {
      os << rung.step_in << ",";
      if (std::isfinite(rung.elevation_in)) {
        os << JsonDouble(rung.elevation_in);
      }
      os << ",";
      if (std::isfinite(rung.elevation_delta_in)) {
        os << JsonDouble(rung.elevation_delta_in);
      }
      os << "\n";
    }
    return os.str();
  }

  bool WriteFiles(const std::string& dir, const std::string& stem) const {
    const std::string kJsonPath = dir + "/" + stem + ".json";
    const std::string kCsvPath = dir + "/" + stem + ".csv";
    std::ofstream json_out(kJsonPath.c_str());
    if (!json_out.is_open()) {
      return false;
    }
    json_out << ToJson();
    json_out.close();
    std::ofstream csv_out(kCsvPath.c_str());
    if (!csv_out.is_open()) {
      return false;
    }
    csv_out << ToCsv();
    csv_out.close();
    return true;
  }
};

struct SensitivityRow {
  std::string input;
  uint32_t range_ft = 0;
  std::string output;
  double h_accepted = 0.0;
  double raw_deriv = 0.0;
  double canned_response = 0.0;
  bool nonlinear = false;
  std::string status;
};

struct SensitivityArtifact {
  std::string provenance_lob_version;
  std::string provenance_git_sha;
  std::string solver_config;
  std::vector<SensitivityRow> rows;

  void AddRow(const std::string& input, uint32_t range_ft,
              const std::string& output, double h_accepted, double raw_deriv,
              double canned_response, bool nonlinear,
              const std::string& status) {
    SensitivityRow row;
    row.input = input;
    row.range_ft = range_ft;
    row.output = output;
    row.h_accepted = h_accepted;
    row.raw_deriv = raw_deriv;
    row.canned_response = canned_response;
    row.nonlinear = nonlinear;
    row.status = status;
    rows.push_back(row);
  }

  std::string ToJson() const {
    std::ostringstream os;
    os << R"({"provenance":{"lob_version":")"
       << JsonEscape(provenance_lob_version) << R"(","git_sha":")"
       << JsonEscape(provenance_git_sha) << R"("},"solver_config":")"
       << JsonEscape(solver_config) << R"(","rows":[)";
    bool first = true;
    for (const auto& row : rows) {
      if (!first) {
        os << ",";
      }
      first = false;
      os << R"({"input":")" << JsonEscape(row.input) << R"(","range_ft":)"
         << row.range_ft << R"(,"output":")" << JsonEscape(row.output)
         << R"(","h_accepted":)" << JsonDouble(row.h_accepted)
         << R"(,"raw_deriv":)" << JsonDouble(row.raw_deriv)
         << R"(,"canned_response":)" << JsonDouble(row.canned_response)
         << R"(,"nonlinear":)" << (row.nonlinear ? "true" : "false")
         << R"(,"status":")" << JsonEscape(row.status)
         << R"(")"
         // S reserved for Phase 4 (spec §9.3/§11): semi-elasticity needs
         // u(x) values that do not exist yet; JSON carries the null
         // placeholder while CSV omits the column until Phase 4 fills it.
         << R"(,"sensitivity_coefficient_S":null})";
    }
    os << "]}";
    return os.str();
  }

  std::string ToCsv() const {
    std::ostringstream os;
    os << "input,range_ft,output,h_accepted,raw_deriv,canned_response,"
          "nonlinear,status\n";
    for (const auto& row : rows) {
      os << JsonEscape(row.input) << "," << row.range_ft << ","
         << JsonEscape(row.output) << ",";
      if (std::isfinite(row.h_accepted)) {
        os << JsonDouble(row.h_accepted);
      }
      os << ",";
      if (std::isfinite(row.raw_deriv)) {
        os << JsonDouble(row.raw_deriv);
      }
      os << ",";
      if (std::isfinite(row.canned_response)) {
        os << JsonDouble(row.canned_response);
      }
      os << "," << (row.nonlinear ? "true" : "false") << ","
         << JsonEscape(row.status) << "\n";
    }
    return os.str();
  }

  bool WriteFiles(const std::string& dir, const std::string& stem) const {
    const std::string kJsonPath = dir + "/" + stem + ".json";
    const std::string kCsvPath = dir + "/" + stem + ".csv";
    std::ofstream json_out(kJsonPath.c_str());
    if (!json_out.is_open()) {
      return false;
    }
    json_out << ToJson();
    json_out.close();
    std::ofstream csv_out(kCsvPath.c_str());
    if (!csv_out.is_open()) {
      return false;
    }
    csv_out << ToCsv();
    csv_out.close();
    return true;
  }
};

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
