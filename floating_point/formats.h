#pragma once

#include "floating_point.h"

using SinglePrecision = FloatingPoint<23, 8>;
using HalfPrecision = FloatingPoint<10, 5>;
using E4M3FN = FloatingPoint<3, 4>;
using E5M2 = FloatingPoint<2, 5>;
using E2M1 = FloatingPoint<1, 2>;

template <>
[[nodiscard]] inline bool FloatingPoint<3, 4>::IsNaN() const {
  return exponent_ == kExponentMask && mantissa_ == kMantissaMask;
}

template <>
[[nodiscard]] inline bool FloatingPoint<3, 4>::IsInfinity() const {
  return false;
}

template <>
[[nodiscard]] inline bool FloatingPoint<1, 2>::IsNaN() const {
  return false;
}

template <>
[[nodiscard]] inline bool FloatingPoint<1, 2>::IsInfinity() const {
  return false;
}