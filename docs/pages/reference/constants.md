@page ref_constants Physical Constants

# Physical Constants

Source of truth is `source/constants.hpp` and `source/eng_units.hpp` (`convert::`).
These values are `constexpr` and feed directly into the builder, atmosphere,
unit helpers, and solver. They are documented here rather than scattered through
the source so a reader can audit the physical model without reading every file.

@section ref-const-atm ISA / atmosphere

`source/constants.hpp`:

```text
kPi                               = 3.14159265358979323846
kStandardGravityFtPerSecSq        = 32.17405  ft/s²  (also kLbsPerSlug)
kDegreesPerTurn                   = 360
kSeaLevelDegF                     = 59.0 °F
kSeaLevelHumidityPercent          = 0.0 %
kSeaLevelPressureInHg             = 29.92 inHg
kSeaLevelAirDensityLbsPerCuFt     = 0.0764742 lb/ft³
kSeaLevelSpeedOfSoundFps          = 1116.45 fps
kLapseDegFPerFt                   = 0.00356616 °F/ft
kTropopauseAltitudeFt             = 36 090 ft
kStratosphereAltitudeFt           = 65 617 ft
kMinimumTempDegF                  = -69.7 °F
kGasConstantAir                   = 1716.46 ft·lbf/(slug·°R)
kHydrostaticExponent              = g/(R·L)−1 ≈ 4.2562
kBarometricExponent               = g/(R·L)   ≈ 5.2562
kArmySeaLevelAirDensityLbsPerCuFt = 0.0751265 lb/ft³
kArmyToIcaoBcConversionFactor     = 0.0751265/0.0764742 ≈ 0.982
kAngularVelocityOfEarthRadPerSec  = 7.292115e-5 rad/s
```

`kArmyToIcaoBcConversionFactor` scales G* splines to ICAO; the BC setters
respect `BCAtmosphere` (`source/lob_builder.cpp`). `BarometricFormula` and Lapse
`T(h)=max(T0−L·h, T_min)` are in `source/calc.hpp` (McCoy p. 166); tropopause
then isothermal exponential above 36 090 ft.

@section ref-const-units Units

`source/eng_units.hpp` `convert::`:

```text
Angle: kRadiansPerDegree=π/180, kMoaPerDegree=60, kMoaPerRadian=60/kRadiansPerDegree,
       kMilPerRadian=1000, kMoaPerMil, kIphyPerMoa=1.047
Length: kInchPerFoot=12, kFeetPerYard=3, kMeterPerFoot=0.3048,
        kMmPerFoot=304.8, kCmPerFoot=30.48
Pressure: kInHgPerPa=0.000295…, kInHgPerPsi=2.036…, kInHgPerMillibar=0.02953…
Mass: kGrainsPerLb=7000, kLbsPerSlug=32.17405, kLbsPerKg=2.204623
Sectional density: kLbsmPerSqInPerKgPerSqM=703.069…
Speed: kFpsPerMph=1.4666…, kFpsPerKph=0.9113…, kFpsPerKn=1.6878…
Time: kMsecPerSec=1e3, kUsecPerSec=1e6
Temp: kDegFPerDegC=1.8, kFreezePointDegF=32, kAbsoluteZeroDegF=-459.67
```

All `StrongT` cross-category conversions are explicit `operator Other` via these
constants; helpers like `LobMoaToInch(moa, range_ft)` multiply by them exactly
(`source/lob_convert.cpp`, `include/lob/lob.h`).

@section ref-const-physics Formulas

```text
CalculateCdCoefficient(ρ, BC) = ρ·π / (8·BC·144)   (144 = in²/ft²)  `source/calc.hpp`
CalculateKineticEnergy(v,m)   = m·v²/2            `source/calc.hpp`
CalculateProjectileReferenceArea(D) = π·D²/4
Miller sg = ftp· 30·mass / (twistRatio²·D³·L/D·(1+(L/D)²)) · cbrt(v/2800) · sign(twist)
  with ftp = ρ0/ρ  (`source/calc.hpp` `CalculateMillerTwistRuleCorrectionFactor`)
SpeedOfSound(T) = 49.0223·√T_R  + humidity `1+0.0014·h·p_sat/p0` (`source/calc.hpp` `CalculateSpeedOfSoundInAir`)
Air density ratio = (p/p0)·(T0_R/T_R)  and `ρ_corr = 1−0.00378·h·p_sat/p0` (McCoy p. 167)
```

Coriolis terms are `2Ω·cos(lat)·sin(az)` etc. in `BuildCoriolis`
(`source/lob_builder.cpp`), `DsDx` in `source/solve_step.cpp`.
