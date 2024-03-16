
// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2024  Joel Benway
// Please see end of file for extended copyright information

#include "ode.hpp"

#include <utility>

#include "cartesian.hpp"
#include "eng_units.hpp"

namespace lob {

SpvT::SpvT() : position(FeetT{0}), velocity(FpsT{0}) {}
SpvT::SpvT(CartesianT<FeetT> p, CartesianT<FpsT> v)
    : position(std::move(p)), velocity(std::move(v)) {}
SpvT::SpvT(const SpvT& other) = default;
SpvT::SpvT(SpvT&& other) noexcept = default;
SpvT& SpvT::operator=(const SpvT& rhs) {
  if (this != &rhs) {
    position = rhs.position;
    velocity = rhs.velocity;
  }
  return *this;
}
SpvT& SpvT::operator=(SpvT&& rhs) noexcept {
  if (this != &rhs) {
    position = rhs.position;
    velocity = rhs.velocity;
    rhs.position = CartesianT<FeetT>(FeetT(0));
    rhs.velocity = CartesianT<FpsT>(FpsT(0));
  }
  return *this;
}

SpvT SpvT::operator+(const SpvT& rhs) const {
  return SpvT{position + rhs.position, velocity + rhs.velocity};
}
SpvT SpvT::operator+(const double& rhs) const {
  return SpvT{position + FeetT(rhs), velocity + FpsT(rhs)};
}
SpvT SpvT::operator*(const SpvT& rhs) const {
  return SpvT{position + rhs.position, velocity * rhs.velocity};
}
SpvT SpvT::operator*(const double& rhs) const {
  return SpvT{position * FeetT(rhs), velocity * FpsT(rhs)};
}

CartesianT<FeetT> SpvT::P() const { return position; }
void SpvT::P(FeetT input) { position = CartesianT<FeetT>(input); }
void SpvT::P(double input) { position = CartesianT<FeetT>(FeetT(input)); }
CartesianT<FpsT> SpvT::V() const { return velocity; }
void SpvT::V(FpsT input) { velocity = CartesianT<FpsT>(input); }
void SpvT::V(double input) { velocity = CartesianT<FpsT>(FpsT(input)); }

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