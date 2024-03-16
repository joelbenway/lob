# This file is a part of lob, an exterior ballistics calculation library
# Copyright (c) 2024  Joel Benway
# Please see end of file for extended copyright information
{
  description = "A Nix-flake-based C++ Development Environment";

  inputs.nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";

  outputs = { self, nixpkgs }:
    let
      supportedSystems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forEachSupportedSystem = f: nixpkgs.lib.genAttrs supportedSystems (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
    {
      devShells = forEachSupportedSystem ({ pkgs }: {
        default = pkgs.mkShell.override {
          # Override stdenv in order to change compiler:
          # stdenv = pkgs.clangStdenv;
        }
        {
          packages = with pkgs; [
            # Development Tools
            clang-tools
            cmake
            codespell
            cppcheck
            doxygen
            # gbenchmark
            lcov
            mold
            ninja # not used yet.
          ] ++ (if system == "aarch64-darwin" then [ ] else [ gdb ]);
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