// Copyright (c) 2026  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <string>

namespace tests {

TEST(ReferenceJson, ParsesFlatCaseShape) {
  const nlohmann::json kRoot = nlohmann::json::parse(
      "{\"id\":\"ref-icao\",\"ranges_ft\":[0,150],\"expected\":[{\"range_ft\":0,"
      "\"velocity_fps\":2800,\"u_ref\":\"unknown\"}],\"status\":\"provisional\","
      "\"supersedes\":null}");
  EXPECT_EQ(kRoot.at("id").get<std::string>(), "ref-icao");
  EXPECT_DOUBLE_EQ(kRoot.at("ranges_ft").at(1).get<double>(), 150.0);
  EXPECT_DOUBLE_EQ(
      kRoot.at("expected").at(0).at("velocity_fps").get<double>(), 2800.0);
  EXPECT_EQ(kRoot.at("expected").at(0).at("u_ref").get<std::string>(),
            "unknown");
  EXPECT_TRUE(kRoot.at("supersedes").is_null());
  EXPECT_EQ(kRoot.at("status").get<std::string>(), "provisional");
}

TEST(ReferenceJson, RejectsMalformedDocuments) {
  for (const char* kBad : {"{\"a\":1,}", "{\"a\":", "{\"a\" 1}", "[1,2",
                           "{\"a\":01}", ""}) {
    EXPECT_THROW(nlohmann::json::parse(kBad), nlohmann::json::parse_error)
        << "accepted: " << kBad;
  }
}

TEST(ReferenceJson, MissingKeysThrowInsteadOfInserting) {
  const nlohmann::json kRoot = nlohmann::json::parse("{\"a\":1}");
  EXPECT_THROW(kRoot.at("zzz"), nlohmann::json::out_of_range);
  EXPECT_THROW(kRoot.at("a").get<std::string>(), nlohmann::json::type_error);
}

}  // namespace tests
