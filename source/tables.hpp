// This file is a part of lob, an exterior ballistics calculation library
// Copyright (c) 2025  Joel Benway
// Please see end of file for extended copyright information

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "eng_units.hpp"

namespace lob {

static constexpr size_t kTableSize = 85;
static constexpr uint16_t kTableScale = 10'000;

constexpr std::array<uint16_t, kTableSize> kMachs = {
    0U,     500U,   1000U,  1500U,  2000U,  2500U,  3000U,  3500U,  4000U,
    4500U,  5000U,  5500U,  6000U,  6500U,  7000U,  7250U,  7500U,  7750U,
    8000U,  8250U,  8500U,  8750U,  9000U,  9250U,  9500U,  9750U,  10000U,
    10250U, 10500U, 10750U, 11000U, 11250U, 11500U, 12000U, 12500U, 13000U,
    13500U, 14000U, 14500U, 15000U, 15500U, 16000U, 16500U, 17000U, 17500U,
    18000U, 18500U, 19000U, 19500U, 20000U, 20500U, 21000U, 21500U, 22000U,
    22500U, 23000U, 23500U, 24000U, 24500U, 25000U, 25500U, 26000U, 26500U,
    27000U, 27500U, 28000U, 28500U, 29000U, 29500U, 30000U, 31000U, 32000U,
    33000U, 34000U, 35000U, 36000U, 37000U, 38000U, 39000U, 40000U, 42000U,
    44000U, 46000U, 48000U, 50000U};

constexpr std::array<uint16_t, kTableSize> kG1Drags = {
    2629U, 2558U, 2487U, 2413U, 2344U, 2278U, 2214U, 2155U, 2104U, 2061U, 2032U,
    2020U, 2034U, 2082U, 2165U, 2230U, 2313U, 2417U, 2546U, 2706U, 2901U, 3136U,
    3415U, 3734U, 4084U, 4448U, 4805U, 5136U, 5427U, 5677U, 5883U, 6053U, 6191U,
    6393U, 6518U, 6589U, 6621U, 6625U, 6607U, 6573U, 6528U, 6474U, 6413U, 6347U,
    6280U, 6210U, 6141U, 6072U, 6003U, 5934U, 5867U, 5804U, 5743U, 5685U, 5630U,
    5577U, 5527U, 5481U, 5438U, 5397U, 5359U, 5325U, 5293U, 5264U, 5237U, 5211U,
    5188U, 5168U, 5150U, 5133U, 5105U, 5084U, 5067U, 5054U, 5040U, 5030U, 5022U,
    5016U, 5010U, 5006U, 4998U, 4995U, 4992U, 4990U, 4988U};

constexpr std::array<uint16_t, kTableSize> kG2Drags = {
    2303U, 2298U, 2287U, 2271U, 2251U, 2227U, 2196U, 2156U, 2107U, 2048U, 1980U,
    1905U, 1828U, 1758U, 1702U, 1683U, 1669U, 1664U, 1667U, 1682U, 1711U, 1761U,
    1831U, 2004U, 2589U, 3492U, 3983U, 4075U, 4103U, 4114U, 4106U, 4089U, 4068U,
    4021U, 3966U, 3904U, 3835U, 3759U, 3678U, 3594U, 3512U, 3432U, 3356U, 3282U,
    3213U, 3149U, 3089U, 3033U, 2982U, 2933U, 2889U, 2846U, 2806U, 2768U, 2731U,
    2696U, 2663U, 2632U, 2602U, 2572U, 2543U, 2515U, 2487U, 2460U, 2433U, 2408U,
    2382U, 2357U, 2333U, 2309U, 2262U, 2217U, 2173U, 2132U, 2091U, 2052U, 2014U,
    1978U, 1944U, 1912U, 1851U, 1794U, 1741U, 1693U, 1648U};

constexpr std::array<uint16_t, kTableSize> kG5Drags = {
    1710U, 1719U, 1727U, 1732U, 1734U, 1730U, 1718U, 1696U, 1668U, 1637U, 1603U,
    1566U, 1529U, 1497U, 1473U, 1466U, 1463U, 1471U, 1489U, 1527U, 1583U, 1672U,
    1815U, 2051U, 2413U, 2884U, 3379U, 3785U, 4032U, 4147U, 4201U, 4245U, 4278U,
    4338U, 4373U, 4392U, 4403U, 4406U, 4401U, 4386U, 4362U, 4328U, 4286U, 4237U,
    4182U, 4121U, 4057U, 3991U, 3926U, 3861U, 3800U, 3741U, 3684U, 3630U, 3578U,
    3529U, 3481U, 3435U, 3391U, 3349U, 3308U, 3269U, 3231U, 3194U, 3159U, 3125U,
    3092U, 3060U, 3029U, 2999U, 2942U, 2889U, 2838U, 2790U, 2745U, 2703U, 2662U,
    2624U, 2588U, 2553U, 2488U, 2429U, 2376U, 2326U, 2280U};

constexpr std::array<uint16_t, kTableSize> kG6Drags = {
    2617U, 2553U, 2491U, 2432U, 2376U, 2324U, 2278U, 2238U, 2205U, 2177U, 2155U,
    2138U, 2126U, 2121U, 2122U, 2126U, 2132U, 2141U, 2154U, 2172U, 2194U, 2229U,
    2297U, 2449U, 2732U, 3141U, 3597U, 3994U, 4261U, 4402U, 4465U, 4490U, 4497U,
    4482U, 4441U, 4390U, 4336U, 4279U, 4221U, 4162U, 4102U, 4042U, 3981U, 3919U,
    3855U, 3788U, 3721U, 3652U, 3583U, 3515U, 3447U, 3381U, 3314U, 3249U, 3185U,
    3122U, 3060U, 3000U, 2941U, 2883U, 2827U, 2772U, 2719U, 2668U, 2620U, 2574U,
    2530U, 2487U, 2446U, 2407U, 2333U, 2265U, 2202U, 2144U, 2089U, 2039U, 1991U,
    1947U, 1905U, 1866U, 1794U, 1730U, 1673U, 1621U, 1574U};

constexpr std::array<uint16_t, kTableSize> kG7Drags = {
    1198U, 1197U, 1196U, 1194U, 1193U, 1194U, 1194U, 1194U, 1193U, 1193U, 1194U,
    1193U, 1194U, 1197U, 1202U, 1207U, 1215U, 1226U, 1242U, 1266U, 1306U, 1368U,
    1464U, 1660U, 2054U, 2993U, 3803U, 4015U, 4043U, 4034U, 4014U, 3987U, 3955U,
    3884U, 3810U, 3732U, 3657U, 3580U, 3508U, 3440U, 3376U, 3315U, 3260U, 3209U,
    3160U, 3117U, 3078U, 3042U, 3010U, 2980U, 2951U, 2922U, 2892U, 2864U, 2835U,
    2807U, 2779U, 2752U, 2725U, 2697U, 2670U, 2643U, 2615U, 2588U, 2561U, 2533U,
    2506U, 2479U, 2451U, 2424U, 2368U, 2313U, 2258U, 2205U, 2154U, 2106U, 2060U,
    2017U, 1975U, 1935U, 1861U, 1793U, 1730U, 1672U, 1618U};

constexpr std::array<uint16_t, kTableSize> kG8Drags = {
    2105U, 2105U, 2104U, 2104U, 2103U, 2103U, 2103U, 2103U, 2103U, 2102U, 2102U,
    2102U, 2102U, 2102U, 2103U, 2103U, 2103U, 2103U, 2104U, 2104U, 2105U, 2106U,
    2109U, 2183U, 2571U, 3358U, 4068U, 4378U, 4476U, 4493U, 4477U, 4450U, 4419U,
    4353U, 4283U, 4208U, 4133U, 4059U, 3986U, 3915U, 3845U, 3777U, 3710U, 3645U,
    3581U, 3519U, 3458U, 3400U, 3343U, 3288U, 3234U, 3182U, 3131U, 3081U, 3032U,
    2983U, 2937U, 2891U, 2845U, 2802U, 2760U, 2720U, 2681U, 2642U, 2605U, 2569U,
    2534U, 2499U, 2465U, 2432U, 2368U, 2308U, 2251U, 2197U, 2147U, 2101U, 2058U,
    2019U, 1983U, 1950U, 1890U, 1837U, 1791U, 1750U, 1713U};

template <typename T>
double LobLerp(const T* x_lut, const T* y_lut, size_t size, double x_in);

template <typename T, size_t N>
double LobLerp(const std::array<T, N>& x_lut, const std::array<T, N>& y_lut,
               const double x_in) {
  return LobLerp(x_lut.data(), y_lut.data(), N, x_in);
}

template <size_t N>
double LobLerp(const std::array<uint16_t, N>& x_lut,
               const std::array<uint16_t, N>& y_lut, MachT x_in) {
  const double kX = x_in.Value() * kTableScale;
  return LobLerp(x_lut.data(), y_lut.data(), N, kX) / kTableScale;
}

template <typename T>
double LobQerp(const T* x_lut, const T* y_lut, size_t size, double x_in);

template <typename T, size_t N>
double LobQerp(const std::array<T, N>& x_lut, const std::array<T, N>& y_lut,
               const double x_in) {
  return LobQerp(x_lut.data(), y_lut.data(), N, x_in);
}

template <typename T>
void ResizeMachDragTable(const T* pmachs, const T* pdrags, size_t* indices,
                         size_t old_size, T* pnew_machs, T* pnew_drags,
                         size_t new_size);

template <typename T, size_t OldSize, size_t NewSize>
void ResizeMachDragTable(const std::array<T, OldSize>& machs,
                         const std::array<T, OldSize>& drags,
                         std::array<T, NewSize>* pnew_machs,
                         std::array<T, NewSize>* pnew_drags) {
  std::array<size_t, OldSize> indices = {};
  ResizeMachDragTable(machs.data(), drags.data(), indices.data(), OldSize,
                      pnew_machs->data(), pnew_drags->data(), NewSize);
}

namespace help {

struct Point {
  double x{0};
  double y{0};
};

struct Circle {
  Point center;
  double radius{0};
};

double CalculatePerpendicularSlope(double slope);

Circle FitCircle(const Point& p1, const Point& p2, const Point& p3);

double FindAngleToPointOnCircle(Point p, Circle c);

template <typename T>
void ExpandMachDragTable(const T* pmachs, const T* pdrags, size_t old_size,
                         T* pnew_machs, T* pnew_drags, size_t new_size);

template <typename T>
void CompressMachDragTable(const T* pmachs, const T* pdrags, size_t* indices,
                           size_t old_size, T* pnew_machs, T* pnew_drags,
                           size_t new_size);

}  // namespace help
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