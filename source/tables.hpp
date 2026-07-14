// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>

#include "eng_units.hpp"

namespace lob {
namespace dragtable {

constexpr size_t kTableSize = 87;

constexpr std::array<float, kTableSize> kMachs = {
    0.00000F, 0.05000F, 0.10000F, 0.15000F, 0.20000F, 0.25000F, 0.30000F,
    0.35000F, 0.40000F, 0.45000F, 0.50000F, 0.55000F, 0.60000F, 0.65000F,
    0.70000F, 0.72500F, 0.75000F, 0.77500F, 0.80000F, 0.82500F, 0.85000F,
    0.87500F, 0.90000F, 0.92500F, 0.95000F, 0.97500F, 1.00000F, 1.02500F,
    1.05000F, 1.07500F, 1.10000F, 1.12500F, 1.15000F, 1.17500F, 1.20000F,
    1.22500F, 1.25000F, 1.30000F, 1.35000F, 1.40000F, 1.45000F, 1.50000F,
    1.55000F, 1.60000F, 1.65000F, 1.70000F, 1.75000F, 1.80000F, 1.85000F,
    1.90000F, 1.95000F, 2.00000F, 2.05000F, 2.10000F, 2.15000F, 2.20000F,
    2.25000F, 2.30000F, 2.35000F, 2.40000F, 2.45000F, 2.50000F, 2.55000F,
    2.60000F, 2.65000F, 2.70000F, 2.75000F, 2.80000F, 2.85000F, 2.90000F,
    2.95000F, 3.00000F, 3.10000F, 3.20000F, 3.30000F, 3.40000F, 3.50000F,
    3.60000F, 3.70000F, 3.80000F, 3.90000F, 4.00000F, 4.20000F, 4.40000F,
    4.60000F, 4.80000F, 5.00000F};

constexpr std::array<float, kTableSize> kG1Drags = {
    0.26290F, 0.25580F, 0.24870F, 0.24130F, 0.23440F, 0.22780F, 0.22140F,
    0.21550F, 0.21040F, 0.20610F, 0.20320F, 0.20200F, 0.20340F, 0.20816F,
    0.21650F, 0.22300F, 0.23130F, 0.24170F, 0.25460F, 0.27060F, 0.29010F,
    0.31360F, 0.34150F, 0.37340F, 0.40840F, 0.44480F, 0.48050F, 0.51360F,
    0.54270F, 0.56770F, 0.58830F, 0.60530F, 0.61910F, 0.63024F, 0.63930F,
    0.64635F, 0.65180F, 0.65890F, 0.66210F, 0.66250F, 0.66070F, 0.65730F,
    0.65280F, 0.64740F, 0.64130F, 0.63470F, 0.62800F, 0.62100F, 0.61410F,
    0.60720F, 0.60030F, 0.59340F, 0.58670F, 0.58040F, 0.57430F, 0.56850F,
    0.56300F, 0.55770F, 0.55270F, 0.54810F, 0.54380F, 0.53970F, 0.53596F,
    0.53250F, 0.52933F, 0.52640F, 0.52363F, 0.52110F, 0.51884F, 0.51680F,
    0.51496F, 0.51330F, 0.51050F, 0.50840F, 0.50670F, 0.50540F, 0.50400F,
    0.50300F, 0.50220F, 0.50160F, 0.50100F, 0.50060F, 0.49980F, 0.49950F,
    0.49920F, 0.49900F, 0.49880F};

constexpr std::array<float, kTableSize> kG2Drags = {
    0.23030F, 0.22980F, 0.22870F, 0.22710F, 0.22510F, 0.22270F, 0.21960F,
    0.21560F, 0.21070F, 0.20480F, 0.19800F, 0.19050F, 0.18280F, 0.17580F,
    0.17020F, 0.16821F, 0.16690F, 0.16640F, 0.16670F, 0.16820F, 0.17110F,
    0.17610F, 0.18310F, 0.20040F, 0.25890F, 0.34920F, 0.39830F, 0.40750F,
    0.41030F, 0.41140F, 0.41060F, 0.40890F, 0.40680F, 0.40460F, 0.40210F,
    0.39943F, 0.39660F, 0.39040F, 0.38350F, 0.37590F, 0.36780F, 0.35940F,
    0.35120F, 0.34320F, 0.33560F, 0.32820F, 0.32130F, 0.31490F, 0.30890F,
    0.30330F, 0.29820F, 0.29330F, 0.28890F, 0.28460F, 0.28060F, 0.27680F,
    0.27310F, 0.26960F, 0.26630F, 0.26320F, 0.26020F, 0.25720F, 0.25430F,
    0.25150F, 0.24870F, 0.24600F, 0.24330F, 0.24080F, 0.23820F, 0.23570F,
    0.23330F, 0.23090F, 0.22620F, 0.22170F, 0.21730F, 0.21320F, 0.20910F,
    0.20520F, 0.20140F, 0.19780F, 0.19440F, 0.19120F, 0.18510F, 0.17940F,
    0.17410F, 0.16930F, 0.16480F};

constexpr std::array<float, kTableSize> kG5Drags = {
    0.17100F, 0.17190F, 0.17270F, 0.17320F, 0.17340F, 0.17300F, 0.17180F,
    0.16960F, 0.16680F, 0.16370F, 0.16030F, 0.15660F, 0.15290F, 0.14970F,
    0.14730F, 0.14662F, 0.14630F, 0.14709F, 0.14890F, 0.15252F, 0.15830F,
    0.16720F, 0.18150F, 0.20510F, 0.24130F, 0.28840F, 0.33790F, 0.37850F,
    0.40320F, 0.41470F, 0.42010F, 0.42425F, 0.42780F, 0.43109F, 0.43380F,
    0.43579F, 0.43730F, 0.43920F, 0.44030F, 0.44060F, 0.44010F, 0.43860F,
    0.43620F, 0.43280F, 0.42860F, 0.42370F, 0.41820F, 0.41210F, 0.40570F,
    0.39910F, 0.39260F, 0.38610F, 0.38000F, 0.37410F, 0.36840F, 0.36300F,
    0.35780F, 0.35290F, 0.34810F, 0.34350F, 0.33910F, 0.33490F, 0.33084F,
    0.32690F, 0.32308F, 0.31940F, 0.31589F, 0.31250F, 0.30920F, 0.30600F,
    0.30290F, 0.29990F, 0.29420F, 0.28890F, 0.28380F, 0.27900F, 0.27450F,
    0.27030F, 0.26620F, 0.26240F, 0.25880F, 0.25530F, 0.24880F, 0.24290F,
    0.23760F, 0.23260F, 0.22800F};

constexpr std::array<float, kTableSize> kG6Drags = {
    0.26170F, 0.25530F, 0.24910F, 0.24320F, 0.23760F, 0.23240F, 0.22780F,
    0.22380F, 0.22050F, 0.21770F, 0.21550F, 0.21380F, 0.21260F, 0.21210F,
    0.21220F, 0.21255F, 0.21320F, 0.21412F, 0.21540F, 0.21710F, 0.21940F,
    0.22290F, 0.22970F, 0.24490F, 0.27320F, 0.31410F, 0.35970F, 0.39940F,
    0.42610F, 0.44020F, 0.44650F, 0.44900F, 0.44970F, 0.44940F, 0.44820F,
    0.44640F, 0.44410F, 0.43900F, 0.43360F, 0.42790F, 0.42210F, 0.41620F,
    0.41020F, 0.40420F, 0.39810F, 0.39190F, 0.38550F, 0.37880F, 0.37210F,
    0.36520F, 0.35830F, 0.35150F, 0.34470F, 0.33810F, 0.33140F, 0.32490F,
    0.31850F, 0.31220F, 0.30600F, 0.30000F, 0.29410F, 0.28830F, 0.28267F,
    0.27720F, 0.27189F, 0.26680F, 0.26200F, 0.25740F, 0.25296F, 0.24870F,
    0.24462F, 0.24070F, 0.23330F, 0.22650F, 0.22020F, 0.21440F, 0.20890F,
    0.20390F, 0.19910F, 0.19470F, 0.19050F, 0.18660F, 0.17940F, 0.17300F,
    0.16730F, 0.16210F, 0.15740F};

constexpr std::array<float, kTableSize> kG7Drags = {
    0.11980F, 0.11970F, 0.11960F, 0.11940F, 0.11930F, 0.11940F, 0.11940F,
    0.11940F, 0.11930F, 0.11930F, 0.11940F, 0.11930F, 0.11940F, 0.11970F,
    0.12020F, 0.12070F, 0.12150F, 0.12260F, 0.12420F, 0.12660F, 0.13060F,
    0.13680F, 0.14640F, 0.16600F, 0.20540F, 0.29930F, 0.38030F, 0.40150F,
    0.40430F, 0.40340F, 0.40140F, 0.39870F, 0.39550F, 0.39202F, 0.38840F,
    0.38474F, 0.38100F, 0.37320F, 0.36570F, 0.35800F, 0.35082F, 0.34400F,
    0.33760F, 0.33150F, 0.32600F, 0.32090F, 0.31600F, 0.31170F, 0.30780F,
    0.30420F, 0.30100F, 0.29800F, 0.29510F, 0.29220F, 0.28920F, 0.28640F,
    0.28350F, 0.28070F, 0.27790F, 0.27520F, 0.27250F, 0.26970F, 0.26700F,
    0.26430F, 0.26150F, 0.25880F, 0.25610F, 0.25330F, 0.25060F, 0.24790F,
    0.24510F, 0.24240F, 0.23680F, 0.23130F, 0.22580F, 0.22050F, 0.21540F,
    0.21060F, 0.20600F, 0.20170F, 0.19750F, 0.19350F, 0.18610F, 0.17930F,
    0.17300F, 0.16720F, 0.16180F};

constexpr std::array<float, kTableSize> kG8Drags = {
    0.21050F, 0.21050F, 0.21040F, 0.21040F, 0.21030F, 0.21030F, 0.21030F,
    0.21030F, 0.21030F, 0.21020F, 0.21020F, 0.21020F, 0.21020F, 0.21020F,
    0.21030F, 0.21030F, 0.21030F, 0.21035F, 0.21040F, 0.21040F, 0.21050F,
    0.21060F, 0.21090F, 0.21830F, 0.25710F, 0.33580F, 0.40680F, 0.43780F,
    0.44760F, 0.44930F, 0.44770F, 0.44500F, 0.44190F, 0.43865F, 0.43530F,
    0.43186F, 0.42830F, 0.42080F, 0.41330F, 0.40590F, 0.39860F, 0.39150F,
    0.38450F, 0.37770F, 0.37100F, 0.36450F, 0.35810F, 0.35190F, 0.34580F,
    0.34000F, 0.33430F, 0.32880F, 0.32340F, 0.31820F, 0.31310F, 0.30810F,
    0.30320F, 0.29830F, 0.29370F, 0.28910F, 0.28450F, 0.28020F, 0.27605F,
    0.27200F, 0.26804F, 0.26420F, 0.26050F, 0.25690F, 0.25336F, 0.24990F,
    0.24651F, 0.24320F, 0.23680F, 0.23080F, 0.22510F, 0.21970F, 0.21470F,
    0.21010F, 0.20580F, 0.20190F, 0.19830F, 0.19500F, 0.18900F, 0.18370F,
    0.17910F, 0.17500F, 0.17130F};

template <typename T>
constexpr double LobLerp(const T* x_lut, const T* y_lut, const size_t size,
                         const double x_in) {
  assert(!(x_in < 0.0) && "input is not negative");
  assert(x_lut != nullptr && y_lut != nullptr && "Input arrays cannot be null");

  if (x_in >= static_cast<double>(x_lut[size - 1])) {
    return static_cast<double>(y_lut[size - 1]);
  }

  size_t low = 0;
  size_t high = size - 1;
  size_t index = 0;

  while (low <= high) {
    const size_t kMid = low + ((high - low) / 2);
    if (static_cast<double>(x_lut[kMid]) <= x_in) {
      index = kMid;
      low = kMid + 1;
    } else {
      high = kMid - 1;
    }
  }

  const auto kX0 = static_cast<double>(x_lut[index]);
  const auto kX1 = static_cast<double>(x_lut[index + 1]);
  const auto kY0 = static_cast<double>(y_lut[index]);
  const auto kY1 = static_cast<double>(y_lut[index + 1]);
  const auto kDx = kX1 - kX0;
  assert(kDx > 0.0 && "x values must be increasing");
  const double kT = (x_in - kX0) / kDx;
  return kY0 + (kT * (kY1 - kY0));
}

template <typename T, size_t N>
constexpr double LobLerp(const std::array<T, N>& x_lut,
                         const std::array<T, N>& y_lut, const double x_in) {
  return LobLerp(x_lut.data(), y_lut.data(), N, x_in);
}

template <size_t N>
constexpr double LobLerp(const std::array<float, N>& x_lut,
                         const std::array<float, N>& y_lut, MachT x_in) {
  const double kX = x_in.Value();
  return LobLerp(x_lut.data(), y_lut.data(), N, kX);
}

}  // namespace dragtable
}  // namespace lob

// This file is part of lob.
//
// lob is free software: you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// lob is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
// A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// lob. If not, see <https://www.gnu.org/licenses/>.
