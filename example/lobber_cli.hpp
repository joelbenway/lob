#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace example {

struct CliConfig {
  bool show_help = false;
  bool show_version = false;
  bool json_mode = false;
  std::optional<std::string> save_input_path;
};

CliConfig ParseArgs(int argc, char* argv[]);

void PrintHelp();
void PrintVersion();

}  // namespace example
