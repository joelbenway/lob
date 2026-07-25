// Copyright (c) 2025  Joel Benway
// SPDX-License-Identifier: GPL-3.0-or-later
// Please see end of file for extended copyright information

#pragma once

#include "eng_units.hpp"
#include "lob/lob.h"
#include "ode.hpp"
#include "splines.hpp"

namespace lob {

void SolveStep(TrajectoryStateT* ps, SecT* pt, spline::CurveView* pcurve,
               const ::LobContext& ctx);

// Distance mode: step is a fixed 1 foot. Advances tof by kStep / vx.
void SolveStep(TrajectoryStateT* ps, FeetT* px, spline::CurveView* pcurve,
               const LobContext& ctx);

TrajectoryStateT DSlopeDt(const TrajectoryStateT& s, const LobContext& ctx,
                          spline::CurveView& curve);

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