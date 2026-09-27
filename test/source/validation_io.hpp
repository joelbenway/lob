// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only artifact writer (Phase 1). Hand-rolled: the project ships zero
// dependencies, and only the writer side is needed until Phase 3 (which adds
// the reference-data reader). C++14, no exceptions from this header itself.

#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace tests {

inline std::string JsonEscape(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (size_t i = 0; i < s.size(); ++i) {
    const char kC = s[i];
    if (kC == '"' || kC == '\\') {
      out += '\\';
    }
    out += kC;
  }
  return out;
}

// NaN/non-finite -> null (CSV uses empty field instead, see ToCsv).
inline std::string JsonDouble(double v) {
  if (!std::isfinite(v)) {
    return "null";
  }
  char buf[32] = {};
  std::snprintf(buf, sizeof(buf), "%.10g", v);
  return std::string(buf);
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
    os << "{\"provenance\":{\"lob_version\":\""
       << JsonEscape(provenance_lob_version) << "\",\"git_sha\":\""
       << JsonEscape(provenance_git_sha) << "\"},\"solver_config\":\""
       << JsonEscape(solver_config) << "\",\"rungs\":[";
    for (size_t i = 0; i < rungs.size(); ++i) {
      if (i > 0) {
        os << ",";
      }
      os << "{\"step_in\":" << rungs[i].step_in
         << ",\"elevation_in\":" << JsonDouble(rungs[i].elevation_in)
         << ",\"elevation_delta_in\":"
         << JsonDouble(rungs[i].elevation_delta_in) << "}";
    }
    os << "]}";
    return os.str();
  }

  std::string ToCsv() const {
    std::ostringstream os;
    os << "step_in,elevation_in,elevation_delta_in\n";
    for (size_t i = 0; i < rungs.size(); ++i) {
      os << rungs[i].step_in << ",";
      if (std::isfinite(rungs[i].elevation_in)) {
        os << JsonDouble(rungs[i].elevation_in);
      }
      os << ",";
      if (std::isfinite(rungs[i].elevation_delta_in)) {
        os << JsonDouble(rungs[i].elevation_delta_in);
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

}  // namespace tests
