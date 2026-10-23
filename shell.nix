# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information
{pkgs}: let
  baseShell =
    pkgs.mkShell.override {
      # Override stdenv in order to change compiler:
      # stdenv = pkgs.clangStdenv;
    }
    {
      packages = with pkgs; [
        # Build/CI Tools
        clang-tools
        cmake
        codespell
        cppcheck
        doxygen
        emscripten
        gbenchmark
        gtest
        lcov
        nlohmann_json
        nodejs
        python3
        valgrind
      ];
      shellHook = let
        projectName = "lob";
        white = "\\[\\033[38;5;015m\\]";
        blue = "\\[\\033[38;5;081m\\]";
        snowflake = "\\342\\235\\204";
        bold = "\\[\\033[01m\\]";
        reset = "\\[$(tput sgr0)\\]";
      in ''
        export PS1="${white}[${reset}${blue}\w${reset}${white}] ${bold}${projectName} ${blue}${snowflake} ${reset}"
        # The nix store copy of emscripten is read-only; its cache must be writable.
        export EM_CACHE="''${TMPDIR:-/tmp}/lob-emscripten-cache"
        mkdir -p "$EM_CACHE"
      '';
    };
in {
  default = baseShell;

  dev = baseShell.overrideAttrs (oldAttrs: let
    extraDevPackages = with pkgs;
      [
        # Extra Development Tools
        act
        bloaty
        cmake-format
        mold-wrapped
        ninja
      ]
      ++ (
        if pkgs.system == "aarch64-darwin"
        then []
        else [gdb]
      );
  in {
    buildInputs = oldAttrs.buildInputs ++ extraDevPackages;
    shellHook = let
      inherit (pkgs) stdenv;
      clangdFile = ".clangd";
      CMakeUserPresetsFile = "CMakeUserPresets.json";
      os =
        if stdenv.isLinux
        then "linux"
        else if stdenv.isDarwin
        then "darwin"
        else "<os>";
      sourceDir = "\\\${sourceDir}";
    in
      ''
        json=$(cat <<-EOF
        {
          "version": 2,
          "cmakeMinimumRequired": {
            "major": 3,
            "minor": 14,
            "patch": 0
          },
          "configurePresets": [
            {
              "name": "dev",
              "binaryDir": "${sourceDir}/build/dev",
              "inherits": ["dev-mode", "ci-${os}"],
              "generator": "Ninja",
              "environment": {
                "CXX_FLAGS_DEV_LINUX": "-Og -g3"
              },
              "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Debug",
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
                "CMAKE_CXX_FLAGS": "\$env{CXX_FLAGS_DEV_LINUX} \$env{LOB_CXX_FLAGS_COMMON}",
                "CMAKE_LINKER_TYPE": "MOLD"
              }
            }
          ],
          "buildPresets": [
            {
              "name": "dev",
              "configurePreset": "dev",
              "configuration": "Debug",
              "jobs": $NIX_BUILD_CORES
            }
          ],
          "testPresets": [
            {
              "name": "dev",
              "configurePreset": "dev",
              "configuration": "Debug",
              "output": {
                "outputOnFailure": true
              },
              "execution": {
                "jobs": $NIX_BUILD_CORES,
                "noTestsAction": "error"
              }
            }
          ]
        }
        EOF
        )

        clangd=$(cat <<-EOF
        CompileFlags:
          CompilationDatabase: build/dev
        EOF
        )

        if [ ! -f ${CMakeUserPresetsFile} ]; then
          echo "$json" > ${CMakeUserPresetsFile}
          echo "${CMakeUserPresetsFile} created successfully"
        else
          echo "${CMakeUserPresetsFile} already exists"
        fi

        if [ ! -f ${clangdFile} ]; then
          echo "$clangd" > ${clangdFile}
          echo "${clangdFile} created successfully"
        else
          echo "${clangdFile} already exists"
        fi
      ''
      + oldAttrs.shellHook;
  });
}
# This file is part of lob.
#
# lob is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# lob is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# lob. If not, see <https://www.gnu.org/licenses/>.
