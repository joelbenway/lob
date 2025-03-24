# This file is a part of lob, an exterior ballistics calculation library
# Copyright (c) 2025  Joel Benway
# Please see end of file for extended copyright information
{
  description = "A Nix-flake-based C++ Development Environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = {
    self,
    nixpkgs,
  }: let
    supportedSystems = ["x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"];
    forEachSupportedSystem = f:
      nixpkgs.lib.genAttrs supportedSystems (system:
        f {
          pkgs = import nixpkgs {
            inherit system;
            # config.allowUnfree = true;
          };
        });
  in {
    devShells = forEachSupportedSystem ({pkgs}: let
      baseShell =
        pkgs.mkShell.override {
          # Override stdenv in order to change compiler:
          # stdenv = pkgs.clangStdenv;
          stdenv = pkgs.gccStdenv;
        }
        {
          packages = with pkgs;
            [
              # Development Tools
              clang-tools
              cmake
              codespell
              cppcheck
              doxygen
              ghostscript
              lcov
              mold
              ninja
              python311
              python311Packages.jinja2
              python311Packages.pygments
            ]
            ++ (
              if system == "aarch64-darwin"
              then []
              else [gdb]
            );
          shellHook = let
            projectName = "lob";
            white = "\\[\\033[38;5;015m\\]";
            blue = "\\[\\033[38;5;081m\\]";
            snowflake = "\\342\\235\\204";
            bold = "\\[\\033[01m\\]";
            reset = "\\[$(tput sgr0)\\]";
          in ''
            export PS1="${white}[${reset}${blue}\w${reset}${white}] ${bold}${projectName} ${blue}${snowflake} ${reset}"
          '';
        };
    in {
      default = baseShell;

      dev = baseShell.overrideAttrs (oldAttrs: {
        shellHook = let
          inherit (pkgs) stdenv;
          filename = "CMakeUserPresets.json";
          os =
            if stdenv.isLinux
            then "linux"
            else if stdenv.isDarwin
            then "darwin"
            else "<os>";
        in
          ''
            cores=$(getconf _NPROCESSORS_ONLN)

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
                  "binaryDir": "/build/dev",
                  "inherits": ["dev-mode", "ci-${os}"],
                  "generator": "Ninja",
                  "cacheVariables": {
                    "CMAKE_BUILD_TYPE": "Debug",
                    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
                    "CMAKE_CXX_FLAGS": "-Og -g3",
                    "CMAKE_LINKER_TYPE": "MOLD"
                  }
                }
              ],
              "buildPresets": [
                {
                  "name": "dev",
                  "configurePreset": "dev",
                  "configuration": "Debug",
                  "jobs": "$cores"
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
                    "jobs": "$cores",
                    "noTestsAction": "error"
                  }
                }
              ]
            }
            EOF
            )

            if [ ! -f ${filename} ]; then
              echo "$json" > ${filename}
              echo "${filename} created successfully"
            else
              echo "${filename} already exists"
            fi
          ''
          + oldAttrs.shellHook;
      });
    });
  };
}
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

