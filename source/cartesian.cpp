// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "cartesian.hpp"

#include <cmath>

#include "eng_units.hpp"

namespace lob {

template <>
FeetT CartesianT<FeetT>::Magnitude() const {
  return FeetT(std::sqrt(std::pow(x_.Value(), 2) + std::pow(y_.Value(), 2) +
                         std::pow(z_.Value(), 2)));
}

template <>
FpsT CartesianT<FpsT>::Magnitude() const {
  return FpsT(std::sqrt(std::pow(x_.Value(), 2) + std::pow(y_.Value(), 2) +
                        std::pow(z_.Value(), 2)));
}

}  // namespace lob

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.