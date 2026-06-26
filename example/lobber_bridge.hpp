#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

#include "lob/lob.hpp"

namespace example {

struct BridgeResult {
  std::vector<lob::Output> outputs;
  size_t count;
};

BridgeResult SolveFromJson(const nlohmann::json& j);
void PrintTable(const lob::Input& input, const lob::Output* outputs, size_t count);
nlohmann::json OutputsToJson(const lob::Output* outputs, size_t count);

}  // namespace example
