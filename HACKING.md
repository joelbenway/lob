# Hacking

Here is some wisdom to help you build and test this project as a developer and potential contributor.

If you plan to contribute, please read the [CONTRIBUTING](CONTRIBUTING.md) guide.

## TL;DR The Nix way

A declarative development environment for this project is available via the [Nix][1] package manager on both Linux and Mac systems through the included [flake](flake.nix). With Nix installed and flakes enabled invoke ```nix develop .#dev``` in the root project directory. Nix will fetch all the same development tools used to author this project and can even generate a `CMakeUserPresets.json` for your system. Included is a build system comprised from [CMake][2], [Ninja][3], and [Mold][4]. Other utilities include [clangd, clang-format, clang-tidy][5], [codespell][6], [cppcheck][7], [doxygen][8], and [lcov][9].

## Developer mode

Build system targets that are only useful for developers of this project are hidden if the `LOB_DEVELOPER_MODE` option is disabled. Enabling this option makes tests and other developer targets and options available. Not enabling this option means that you are a consumer of this project and thus you have no need for these targets and options.

Developer mode is always set to on in CI workflows.

### Presets

This project makes use of [presets][1] to simplify the process of configuring the project. As a developer, you are recommended to always have the [latest CMake version][12] installed to make use of the latest Quality-of-Life additions.

You have a few options to pass `LOB_DEVELOPER_MODE` to the configure command, but this project prefers to use presets.

As a developer, you should create a `CMakeUserPresets.json` file at the root of the project:

```json
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
      "inherits": ["dev-mode", "ci-<os>"],
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug"
    }
  ],
  "testPresets": [
    {
      "name": "dev",
      "configurePreset": "dev",
      "configuration": "Debug",
      "output": {
        "outputOnFailure": true
      }
    }
  ]
}
```

You should replace `<os>` in your newly created presets file with the name of the operating system you have, which may be `win64`, `linux` or `darwin`. You can see what these correspond to in the [`CMakePresets.json`](CMakePresets.json) file.

`CMakeUserPresets.json` is also the perfect place in which you can put all sorts of things that you would otherwise want to pass to the configure command in the terminal.

> **Note**
> Some editors are pretty greedy with how they open projects with presets.
> Some just randomly pick a preset and start configuring without your consent,
> which can be confusing. Make sure that your editor configures when you
> actually want it to, for example in CLion you have to make sure only the
> `dev-dev preset` has `Enable profile` ticked in
> `File > Settings... > Build, Execution, Deployment > CMake` and in Visual
> Studio you have to set the option `Never run configure step automatically`
> in `Tools > Options > CMake` **prior to opening the project**, after which
> you can manually configure using `Project > Configure Cache`.

### Configure, build and test

If you followed the above instructions, then you can configure, build and test the project respectively with the following commands from the project root on any operating system with any build system:

```sh
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev
```

If you are using a compatible editor (e.g. VSCode) or IDE (e.g. CLion, VS), you will also be able to select the above created user presets for automatic integration.

Please note that both the build and test commands accept a `-j` flag to specify the number of jobs to use, which should ideally be specified to the number of threads your CPU has. You may also want to add that to your preset using the `jobs` property, see the [presets documentation][11] for more details.

### Developer mode targets

These are targets you may invoke using the build command from above, with an additional `-t <target>` flag:

#### `coverage`

Available if `ENABLE_COVERAGE` is enabled. This target processes the output of the previously run tests when built with coverage configuration. The commands this target runs can be found in the `COVERAGE_TRACE_COMMAND` and `COVERAGE_HTML_COMMAND` cache variables. The trace command produces an info file by default, which can be submitted to services with CI integration. The HTML command uses the trace command's output to generate an HTML document to `<binary-dir>/coverage_html` by default.

#### `docs`

Available if `BUILD_DOCS` is enabled. Builds to documentation using Doxygen. The output will go to `<binary-dir>/docs` by default (customizable using `DOXYGEN_OUTPUT_DIRECTORY`).

#### `format-check` and `format-fix`

These targets run the clang-format tool on the codebase to check errors and to fix them respectively. Customization available using the `FORMAT_PATTERNS` and `FORMAT_COMMAND` cache variables.

#### `run-examples`

Runs all the examples created by the `add_example` command.

#### `spell-check` and `spell-fix`

These targets run the codespell tool on the codebase to check errors and to fix them respectively. Customization available using the `SPELL_COMMAND` cache variable.

## Running tests on Windows with `BUILD_SHARED_LIBS=ON`

If you are building a shared library on Windows, you must add the path to the DLL file to `PATH` when you want to run tests. One way you could do that is by using PowerShell and writing a script for it, e.g. `env.ps1` at the project root:

```powershell
$oldPrompt = (Get-Command prompt).ScriptBlock

function prompt() { "(Debug) $(& $oldPrompt)" }

$VsInstallPath = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -Property InstallationPath
$Env:Path += ";$VsInstallPath\Common7\IDE;$Pwd\build\dev\Debug"
```

Then you can source this script by running `. env.ps1`. This particular example will only work for Debug builds.

### Passing `PATH` to editors

Make sure you launch your editor of choice from the console with the above script sourced. Look for `(Debug)` in the prompt to confirm, then run e.g. `code .` for VScode or `devenv .` for Visual Studio.

[1]: https://nixos.org/
[2]: https://cmake.org/
[3]: https://ninja-build.org/
[4]: https://github.com/rui314/mold
[5]: https://clang.llvm.org/extra/index.html
[6]: https://github.com/codespell-project/codespell
[7]: http://cppcheck.net/
[8]: https://www.doxygen.nl/index.html
[9]: https://github.com/linux-test-project/lcov
[10]: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
[11]: https://cmake.org/download/
