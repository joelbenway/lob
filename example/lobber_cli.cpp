#include "lobber_cli.hpp"
#include "version.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>

#include "lob/lob.hpp"

namespace example {
namespace {

constexpr std::string_view kHelp = "--help";
constexpr std::string_view kH = "--h";
constexpr std::string_view kVersion = "--version";
constexpr std::string_view kV = "--v";
constexpr std::string_view kJson = "--json";
constexpr std::string_view kSaveInput = "--save-input=";

}  // namespace

CliConfig ParseArgs(int argc, char* argv[]) {
  CliConfig config;
  for (int i = 1; i < argc; ++i) {
    std::string_view arg(argv[i]);
    if (arg == kHelp || arg == kH) {
      config.show_help = true;
    } else if (arg == kVersion || arg == kV) {
      config.show_version = true;
    } else if (arg == kJson) {
      config.json_mode = true;
    } else if (arg.substr(0, kSaveInput.size()) == kSaveInput) {
      config.save_input_path = std::string(arg.substr(kSaveInput.size()));
    }
  }
  return config;
}

void PrintHelp() {
  std::cout << "Usage: lobber [options] [< input.json]\n"
            << "Options:\n"
            << "  --h, --help            Show this help message\n"
            << "  --v, --version         Show version information\n"
            << "  --json                 Output results to stdout in json format\n"
            << "  --save-input=FILE      Save input configuration JSON to FILE\n"
            << "\n"
            << "Note: When run interactively, a wizard prompts for input.\n"
            << "      When stdin is redirected, JSON data is read from stdin.\n"
            << "Example:\n"
            << "\033[33m  lobber --save-input=my_rifle_load.json\n\033[0m"
            << "\n"
            << "Report bugs or give feedback here: "
            << "\033[34mhttps://github.com/joelbenway/lob\033[0m\n";
}

void PrintVersion() {
  std::cout << "Lobber version: " << kProjectVersion << "\n"
            << "Lob version:    " << lob::Version() << "\n\n"
            << "Report bugs or give feedback here: "
            << "\033[34mhttps://github.com/joelbenway/lob\033[0m\n";
}

}  // namespace example
