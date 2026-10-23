# Copyright (c) 2025  Joel Benway
# SPDX-License-Identifier: GPL-3.0-or-later
# Please see end of file for extended copyright information
{
  description = "A Nix-flake-based C++ Development Environment";

  inputs = {
    nixpkgs.url = "https://flakehub.com/f/NixOS/nixpkgs/0.1.*.tar.gz";
  };

  outputs = {
    self,
    nixpkgs,
    ...
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
    packages = forEachSupportedSystem ({pkgs}: let
      cleanSrc = pkgs.lib.cleanSourceWith {
        src = self;
        filter = path: _type:
          !builtins.elem (baseNameOf path)
            ["build" "node_modules" ".superpowers" ".direnv" "prefix" "result"];
      };
    in {
      default = pkgs.stdenv.mkDerivation {
        name = "lob";
        src = cleanSrc;
        nativeBuildInputs = with pkgs; [
          cmake
          gtest
          nlohmann_json
        ];
        configurePhase = ''
          cmake -S . -B build \
            -D CMAKE_BUILD_TYPE=Release \
            -D LOB_DEVELOPER_MODE=ON \
            -D BUILD_EXAMPLES=OFF \
            -D BUILD_BENCHMARKS=OFF \
            -D BUILD_TOOLS=OFF
        '';
        buildPhase = ''
          cmake --build build --parallel $NIX_BUILD_CORES
        '';
        doCheck = true;
        checkPhase = ''
          ctest --test-dir build -j $NIX_BUILD_CORES --output-on-failure
        '';
        installPhase = ''
          cmake --install build --prefix $out
        '';
      };
      lob-wasm = pkgs.stdenv.mkDerivation {
        name = "lob-wasm";
        src = cleanSrc;
        nativeBuildInputs = with pkgs; [
          cmake
          emscripten
          nodejs
          python3
        ];
        configurePhase = ''
          export EM_CACHE="$NIX_BUILD_TOP/emscripten-cache"
          mkdir -p "$EM_CACHE"
          cmake --preset=wasm
        '';
        buildPhase = ''
          cmake --build --preset=wasm --parallel "$NIX_BUILD_CORES"
        '';
        doCheck = true;
        checkPhase = ''
          node --check build/wasm/lob.mjs
          node wasm/smoke.mjs build/wasm/lob.mjs
          node --test wasm/test/*.test.mjs
          node --test example/solver/test/*.test.mjs
          test -f build/wasm/solver/index.html
        '';
        installPhase = ''
          mkdir -p $out/solver
          install -m 0644 build/wasm/index.mjs build/wasm/lob.mjs \
            build/wasm/lob.wasm build/wasm/package.json build/wasm/README.md \
            build/wasm/COPYING.md $out/
          cp -r build/wasm/solver/. $out/solver/
        '';
      };
      lobber = pkgs.stdenv.mkDerivation {
        name = "lobber";
        src = cleanSrc;
        nativeBuildInputs = with pkgs; [
          cmake
          nlohmann_json
        ];
        configurePhase = ''
          cmake -S . -B build \
            -D CMAKE_BUILD_TYPE=Release \
            -D LOB_DEVELOPER_MODE=OFF \
            -D BUILD_EXAMPLES=ON \
            -D BUILD_BENCHMARKS=OFF
        '';
        buildPhase = ''
          cmake --build build --parallel $NIX_BUILD_CORES
        '';
        doCheck = true;
        checkPhase = ''
          ctest --test-dir build --output-on-failure
        '';
        installPhase = ''
          mkdir -p $out/bin
          if [ ! -f build/example/lobber/source/lobber ]; then
            echo "Error: lobber binary not found at build/example/lobber/source/lobber"
            exit 1
          fi
          cp build/example/lobber/source/lobber $out/bin/
        '';
      };
    });
    devShells = forEachSupportedSystem ({pkgs}: import ./shell.nix {inherit pkgs;});
  };
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

