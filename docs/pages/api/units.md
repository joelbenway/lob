@page api_units Units

# Units

@section api-units-overview Overview

lob stores trajectory quantities internally in feet, seconds and pounds, but the
public API speaks the shooter's units.  Internally `source/eng_units.hpp`
provides `StrongT<E,U,T>` — a zero-cost wrapper that makes mismatched units
fail to compile.  Conversion helpers are exposed as C and C++ free functions.

@section api-units-strong Strong types (internal)

`StrongT` is templated on an enum category `E` (Length, Speed, Angle …),
a specific unit `U`, and a representation `T`.  Arithmetic preserves the
category; same-category unit conversions (e.g. `InchT → FeetT`, `MoaT → RadiansT`)
change only the unit via the supplied `operator StrongT<E, Other, T>`; cross-category operations are ill-formed.
Helpers like `IsNaN()`, `Value()`, `U32()`, `U16()` and `Inverse()` are
provided.  The type is `constexpr` where possible.

The strong types eliminate unit mixups inside the integrator and builder, then
disappear at the ABI boundary — `LobContext` fields are plain `double`.

@section api-units-conversions Conversion helpers

Most conversions are exact multiplications by scale factors from
`source/eng_units.hpp` (`convert::` namespace); `LobMoaToInch` and `LobInchToMoa` also depend on `range_ft` and Celsius-to-Fahrenheit includes an additive offset.  Selected helpers
(`include/lob/lob.h`):

- Angle: `LobMoaToMil`, `LobMoaToDeg`, `LobMoaToIphy`, `LobMoaToInch(moa, range_ft)`,
  `LobInchToMoa`, `LobInchToMil`, `LobInchToDeg`, and the `Mil`/`Deg` variants.
- Length: `LobMToYd`, `LobYdToFt`, `LobMToFt`, `LobFtToIn`, `LobMmToIn`, `LobCmToIn`
  and inverses.
- Pressure: `LobPaToInHg`, `LobMbarToInHg`, `LobPsiToInHg`.
- Mass: `LobLbsToGrain`, `LobGToGrain`, `LobKgToGrain`, `LobKgSqMToPmsi`.
- Speed: `LobFpsToMps`, `LobMpsToFps`, `LobKphToMph`, `LobKnToMph`.
- Time: `LobMsToS`, `LobUsToS`, `LobSToMs`, `LobSToUs`.
- Temperature: `LobDegCToDegF`.

Each has a `lob::` inline wrapper (`MoaToMil`, `MToYd`, …) in `lob.hpp`.

@section api-units-roots Roots

Angle conversion constants are rooted at `kPi` (`source/eng_units.hpp`):

```
kRadiansPerDegree = π/180
kMoaPerRadian     = 60 / kRadiansPerDegree
kMilPerRadian     = 1000
kIphyPerMoa       = 1.047
```

Length is rooted at feet: `kInchPerFoot = 12`, `kFeetPerYard = 3`,
`kMeterPerFoot = 0.3048`.  Mass at `kGrainsPerLb = 7000` and
`kLbsPerSlug = 32.17405` (standard gravity).  Time and temperature use the
usual SI/FPS scales.

@section api-units-limits Limitations

- The helpers are pure arithmetic; they do not validate ranges.
- The strong-type system stops at the public ABI.  Callers passing raw
  `double` to the C API must get the units right themselves — prefer the
  `lob::Builder` setters which name the unit (e.g. `DiameterInch`,
  `MassGrains`).
