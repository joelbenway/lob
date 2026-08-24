/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "lob", "index.html", [
    [ "Technical Reference Overview", "index.html", "index" ],
    [ "About", "about.html", [
      [ "About this site", "about.html#about-doxygen", null ]
    ] ],
    [ "Builder", "api_builder.html", [
      [ "Builder", "api_builder.html#autotoc_md0", null ],
      [ "Why a builder", "api_builder.html#api-builder-why", null ],
      [ "Lifecycle", "api_builder.html#api-builder-lifecycle", null ],
      [ "Required vs optional", "api_builder.html#api-builder-required", null ],
      [ "Errors", "api_builder.html#api-builder-errors", null ],
      [ "Drag tables", "api_builder.html#api-builder-tables", null ],
      [ "Thread safety", "api_builder.html#api-builder-thread", null ]
    ] ],
    [ "Context and Results", "api_context.html", [
      [ "Context and Results", "api_context.html#autotoc_md1", null ],
      [ "Context", "api_context.html#api-context-context", null ],
      [ "Output", "api_context.html#api-context-output", null ],
      [ "Error handling", "api_context.html#api-context-errors", null ],
      [ "Lifetime", "api_context.html#api-context-lifetime", null ]
    ] ],
    [ "Errors", "api_errors.html", [
      [ "Errors", "api_errors.html#autotoc_md2", [
        [ "Catalog", "api_errors.html#autotoc_md3", null ]
      ] ],
      [ "Handling", "api_errors.html#api-errors-handling", null ]
    ] ],
    [ "Forward Solve", "api_forward.html", [
      [ "Forward Solve", "api_forward.html#autotoc_md4", null ],
      [ "Signatures", "api_forward.html#api-forward-signatures", null ],
      [ "Output", "api_forward.html#api-forward-output", null ],
      [ "Termination", "api_forward.html#api-forward-stops", null ],
      [ "Preconditions", "api_forward.html#api-forward-errors", null ]
    ] ],
    [ "Inverse Solve", "api_inverse.html", [
      [ "Inverse Solve", "api_inverse.html#autotoc_md5", null ],
      [ "Signatures", "api_inverse.html#api-inverse-signatures", null ],
      [ "What “inverse” means", "api_inverse.html#api-inverse-semantics", null ],
      [ "Two mechanisms", "api_inverse.html#api-inverse-two", null ],
      [ "Which to use", "api_inverse.html#api-inverse-which", null ],
      [ "Edge cases", "api_inverse.html#api-inverse-edge", null ]
    ] ],
    [ "Solver Options", "api_options.html", [
      [ "Solver Options", "api_options.html#autotoc_md6", null ],
      [ "Step size", "api_options.html#api-options-step", null ],
      [ "Maximum time", "api_options.html#api-options-time", null ],
      [ "Minimum speed / energy", "api_options.html#api-options-speed", null ],
      [ "Optic and zero helpers", "api_options.html#api-options-optic", null ],
      [ "Atmosphere, wind, Coriolis", "api_options.html#api-options-atm", null ]
    ] ],
    [ "Units", "api_units.html", [
      [ "Units", "api_units.html#autotoc_md7", null ],
      [ "Overview", "api_units.html#api-units-overview", null ],
      [ "Strong types (internal)", "api_units.html#api-units-strong", null ],
      [ "Conversion helpers", "api_units.html#api-units-conversions", null ],
      [ "Roots", "api_units.html#api-units-roots", null ],
      [ "Limitations", "api_units.html#api-units-limits", null ]
    ] ],
    [ "Atmospheric Model", "model_atmosphere.html", [
      [ "Atmospheric Model", "model_atmosphere.html#autotoc_md8", null ],
      [ "Overview", "model_atmosphere.html#model-atm-overview", null ],
      [ "Inputs", "model_atmosphere.html#model-atm-inputs", null ],
      [ "Formulas", "model_atmosphere.html#model-atm-formulas", null ],
      [ "Output in Context", "model_atmosphere.html#model-atm-output", null ],
      [ "Limitations", "model_atmosphere.html#model-atm-limitations", null ]
    ] ],
    [ "Drag Functions", "model_drag.html", [
      [ "Drag Functions", "model_drag.html#autotoc_md9", null ],
      [ "Standard curves", "model_drag.html#model-drag-std", null ],
      [ "Custom tables", "model_drag.html#model-drag-custom", null ],
      [ "BC/velocity bands", "model_drag.html#model-drag-bands", null ],
      [ "The drag curve in the solver", "model_drag.html#model-drag-curve", null ]
    ] ],
    [ "Point-Mass Model", "model_point_mass.html", [
      [ "Point-Mass Model", "model_point_mass.html#autotoc_md10", null ],
      [ "Equations of motion", "model_point_mass.html#model-pm-equations", null ],
      [ "How it is integrated", "model_point_mass.html#model-pm-integration", null ],
      [ "Assumptions", "model_point_mass.html#model-pm-assumptions", null ]
    ] ],
    [ "Spin Drift and Aerodynamic Jump", "model_spin.html", [
      [ "Spin Drift and Aerodynamic Jump", "model_spin.html#autotoc_md11", null ],
      [ "Spin drift", "model_spin.html#model-spin-drift", null ],
      [ "Aerodynamic jump", "model_spin.html#model-aerodynamic-jump", null ]
    ] ],
    [ "Wind and Coriolis", "model_wind_coriolis.html", [
      [ "Wind and Coriolis", "model_wind_coriolis.html#autotoc_md12", null ],
      [ "Wind vector", "model_wind_coriolis.html#model-wind-vector", null ],
      [ "Usage in the solver", "model_wind_coriolis.html#model-wind-usage", null ],
      [ "Limitations", "model_wind_coriolis.html#model-wind-limitations", null ],
      [ "Coriolis", "model_wind_coriolis.html#model-coriolis", null ]
    ] ],
    [ "Ballistic Coefficient Overview", "bc_overview.html", [
      [ "Ballistic Coefficient", "bc_overview.html#autotoc_md13", null ],
      [ "What a BC is", "bc_overview.html#bc-overview-what", null ],
      [ "Standard curves", "bc_overview.html#bc-overview-std", null ],
      [ "Single BC", "bc_overview.html#bc-overview-single", null ],
      [ "Why bands", "bc_overview.html#bc-overview-why-bands", null ]
    ] ],
    [ "BC/Velocity-Band Transformation", "bc_transformation.html", [
      [ "BC/Velocity-Band Transformation", "bc_transformation.html#autotoc_md14", null ],
      [ "Motivation", "bc_transformation.html#bc-t-motivation", null ],
      [ "Reference drag curve", "bc_transformation.html#bc-t-reference", null ],
      [ "BC/velocity pairs", "bc_transformation.html#bc-t-pairs", null ],
      [ "Transformation", "bc_transformation.html#bc-t-transform", null ],
      [ "Interpolation", "bc_transformation.html#bc-t-interp", null ],
      [ "Limitations and edge cases", "bc_transformation.html#bc-t-limits", null ]
    ] ],
    [ "BC Bands Worked Example", "bc_worked_example.html", [
      [ "BC Bands Worked Example", "bc_worked_example.html#autotoc_md15", null ],
      [ "Resulting drag curve", "bc_worked_example.html#bc-we-figure", null ],
      [ "Worked example — bands to trajectory", "bc_worked_example.html#bc-we-worked", null ],
      [ "Limitations in this example", "bc_worked_example.html#bc-we-limits", null ]
    ] ],
    [ "Design Decisions", "design_overview.html", [
      [ "Design Decisions", "design_overview.html#autotoc_md16", null ],
      [ "C/C++ API architecture", "design_overview.html#design-capi", null ],
      [ "Strong unit types", "design_overview.html#design-units", null ],
      [ "No dynamic allocation", "design_overview.html#design-noalloc", null ],
      [ "Builder pattern", "design_overview.html#design-builder", null ],
      [ "Numerical integration choices", "design_overview.html#design-integration", null ],
      [ "Performance considerations", "design_overview.html#design-perf", null ],
      [ "Shared angle solver — why it matters", "design_overview.html#design-shared", null ]
    ] ],
    [ "Shared Angle Solver", "design_shared_solver.html", [
      [ "Shared Angle Solver", "design_shared_solver.html#autotoc_md17", null ],
      [ "The two places an angle is solved", "design_shared_solver.html#design-shared-problem", null ],
      [ "One solver", "design_shared_solver.html#design-shared-solution", null ],
      [ "Why sharing matters", "design_shared_solver.html#design-shared-why", null ],
      [ "Validation of sharing", "design_shared_solver.html#design-shared-validation", null ]
    ] ],
    [ "Getting Started", "getting_started.html", [
      [ "Getting Started", "getting_started.html#autotoc_md18", null ],
      [ "Building", "getting_started.html#getting_started-build", null ],
      [ "Minimal forward solve", "getting_started.html#getting_started-minimal", null ],
      [ "Richer example", "getting_started.html#getting_started-richer", null ],
      [ "Inverse solve", "getting_started.html#getting_started-inverse", null ],
      [ "Units", "getting_started.html#getting_started-units", null ]
    ] ],
    [ "Inverse Solver", "num_inverse.html", [
      [ "Inverse Solver", "num_inverse.html#autotoc_md19", null ],
      [ "Problem", "num_inverse.html#num-inverse-problem", null ],
      [ "Two APIs, one solver", "num_inverse.html#num-inverse-two", null ],
      [ "Shared angle solver", "num_inverse.html#num-inverse-shared", null ],
      [ "Edges and guarantees", "num_inverse.html#num-inverse-edges", null ],
      [ "Which to call", "num_inverse.html#num-inverse-which", null ]
    ] ],
    [ "ODE Integration", "num_ode.html", [
      [ "ODE Integration", "num_ode.html#autotoc_md20", null ],
      [ "Problem", "num_ode.html#num-ode-why", null ],
      [ "Method", "num_ode.html#num-ode-method", null ],
      [ "Observed accuracy", "num_ode.html#num-ode-accuracy", null ],
      [ "Limitations", "num_ode.html#num-ode-limits", null ]
    ] ],
    [ "Cubic Hermite Splines", "num_splines.html", [
      [ "Cubic Hermite Splines", "num_splines.html#autotoc_md21", null ],
      [ "Why splines", "num_splines.html#num-splines-why", null ],
      [ "Construction", "num_splines.html#num-splines-math", null ],
      [ "Knots", "num_splines.html#num-splines-knots", null ],
      [ "Evaluation", "num_splines.html#num-splines-eval", null ],
      [ "Why not linear", "num_splines.html#num-splines-alternatives", null ]
    ] ],
    [ "Zero-Angle Solver", "num_zero_angle.html", [
      [ "Zero-Angle Solver", "num_zero_angle.html#autotoc_md22", null ],
      [ "Problem", "num_zero_angle.html#num-zero-problem", null ],
      [ "Method", "num_zero_angle.html#num-zero-method", null ],
      [ "Seed", "num_zero_angle.html#num-zero-seed", null ],
      [ "Validation", "num_zero_angle.html#num-zero-validation", null ],
      [ "Reuse", "num_zero_angle.html#num-zero-reuse", null ]
    ] ],
    [ "Physical Constants", "ref_constants.html", [
      [ "Physical Constants", "ref_constants.html#autotoc_md24", null ],
      [ "ISA / atmosphere", "ref_constants.html#ref-const-atm", null ],
      [ "Units", "ref_constants.html#ref-const-units", null ],
      [ "Formulas", "ref_constants.html#ref-const-physics", null ]
    ] ],
    [ "Validation", "validation_overview.html", [
      [ "Validation", "validation_overview.html#autotoc_md25", null ],
      [ "Strategy", "validation_overview.html#validation-strategy", null ],
      [ "Reference data", "validation_overview.html#validation-reference", null ],
      [ "Numerical accuracy", "validation_overview.html#validation-accuracy", null ],
      [ "Invariants", "validation_overview.html#validation-invariants", null ],
      [ "Coverage and checks", "validation_overview.html#validation-coverage", null ],
      [ "Gaps and follow-ups", "validation_overview.html#validation-future", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Functions", "functions_func.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"about.html",
"lob_8h.html#a3fe0194a9f0a0dd6069370dd410b802f",
"num_splines.html#num-splines-why"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';