#pragma once

#include <nlohmann/json.hpp>

namespace example {

nlohmann::json RunWizard();
void PrintGreeting();
bool IsInteractive();

}  // namespace example
