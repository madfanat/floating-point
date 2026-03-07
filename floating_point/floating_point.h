#pragma once

#include <bit>
#include <cstdint>
#include <format>
#include <iostream>
#include <type_traits>

enum class Rounding {
  kTowardZero,
  kTowardNearestEven,
  kTowardPositiveInfinity,
  kTowardNegativeInfinity
};

template <uint32_t kMantissaBits, uint32_t kExponentBits>
class FloatingPoint {
 public:
  using Type = std::conditional_t<
      kMantissaBits + kExponentBits + 1 <= 8, uint8_t,
      std::conditional_t<kMantissaBits + kExponentBits + 1 <= 16, uint16_t,
                         uint32_t>>;

  explicit FloatingPoint(const Type value)
      : mantissa_(value & kMantissaMask),
        exponent_(value >> kMantissaBits & kExponentMask),
        sign_(value >> (kMantissaBits + kExponentBits) & 1) {}

  FloatingPoint(const Type mantissa, const Type exponent, const Type sign)
      : mantissa_(mantissa), exponent_(exponent), sign_(sign) {}

  static void SetRounding(const Rounding rounding) { rounding_ = rounding; }

  FloatingPoint operator+(const FloatingPoint& other) const {
    if (IsNaN()) return MakeQuiet(*this);
    if (other.IsNaN()) return MakeQuiet(other);
    if (IsInfinity()) {
      if (other.IsInfinity() && sign_ != other.sign_) return NaN();
      return *this;
    }
    if (other.IsInfinity()) return other;

    Unpacked a = Unpack();
    Unpacked b = other.Unpack();
    a.mantissa <<= kSignificandBits;
    b.mantissa <<= kSignificandBits;

    return Add(a, b);
  }

  FloatingPoint operator-() const {
    return {mantissa_, exponent_, static_cast<Type>(sign_ ^ 1)};
  }

  FloatingPoint operator-(const FloatingPoint& other) const {
    if (IsNaN()) return MakeQuiet(*this);
    if (other.IsNaN()) return MakeQuiet(other);

    return *this + (-other);
  }

  FloatingPoint operator*(const FloatingPoint& other) const {
    if (IsNaN()) return MakeQuiet(*this);
    if (other.IsNaN()) return MakeQuiet(other);
    uint32_t sign = sign_ ^ other.sign_;
    if (IsInfinity()) {
      if (other.IsZero()) return NaN();
      return sign ? NegativeInfinity() : PositiveInfinity();
    }
    if (other.IsInfinity()) {
      if (IsZero()) return NaN();
      return sign ? NegativeInfinity() : PositiveInfinity();
    }
    if (IsZero() || other.IsZero())
      return sign ? NegativeZero() : PositiveZero();

    Unpacked product = Multiply(*this, other);
    const uint64_t remainder =
        product.mantissa & (1ULL << kSignificandBits) - 1;
    const uint64_t divisor = 1ULL << kSignificandBits;
    const uint32_t mantissa =
        static_cast<uint32_t>(product.mantissa >> kSignificandBits) &
        kMantissaMask;

    return Round(mantissa, remainder, divisor, product.exponent, product.sign);
  }

  FloatingPoint operator/(const FloatingPoint& other) const {
    if (IsNaN()) return MakeQuiet(*this);
    if (other.IsNaN()) return MakeQuiet(other);

    uint32_t sign = sign_ ^ other.sign_;
    if (IsInfinity()) {
      if (other.IsInfinity()) return NaN();
      return sign ? NegativeInfinity() : PositiveInfinity();
    }
    if (other.IsInfinity()) return sign ? NegativeZero() : PositiveZero();
    if (other.IsZero()) {
      if (IsZero()) return NaN();
      return sign ? NegativeInfinity() : PositiveInfinity();
    }
    if (IsZero()) return sign ? NegativeZero() : PositiveZero();

    Unpacked a = Unpack();
    Unpacked b = other.Unpack();

    uint64_t a_mantissa = a.mantissa << kSignificandBits;
    uint64_t quotient = a_mantissa / b.mantissa;
    uint64_t remainder = a_mantissa % b.mantissa;

    int32_t exponent =
        a.exponent - b.exponent + static_cast<int32_t>(kBias) - 1;

    uint64_t divisor;
    if (quotient & 1ULL << kSignificandBits) {
      remainder += (quotient & 1) * b.mantissa;
      divisor = b.mantissa << 1;
      quotient >>= 1;
      ++exponent;
    } else {
      divisor = b.mantissa;
    }

    const uint32_t mantissa = static_cast<uint32_t>(quotient) & kMantissaMask;

    return Round(mantissa, remainder, divisor, exponent, sign);
  }

  static FloatingPoint Mad(const FloatingPoint& a, const FloatingPoint& b,
                           const FloatingPoint& c) {
    return a * b + c;
  }

  static FloatingPoint Fma(const FloatingPoint& a, const FloatingPoint& b,
                           const FloatingPoint& c) {
    if (a.IsNaN()) return MakeQuiet(a);
    if (b.IsNaN()) return MakeQuiet(b);
    if (c.IsNaN()) return MakeQuiet(c);

    uint32_t sign = a.sign_ ^ b.sign_;
    if (a.IsInfinity() || b.IsInfinity()) {
      if (a.IsZero() || b.IsZero()) return NaN();
      if (c.IsInfinity() && sign != c.sign_) return NaN();
      return sign ? NegativeInfinity() : PositiveInfinity();
    }
    if (c.IsInfinity()) return c;

    const Unpacked product =
        a.IsZero() || b.IsZero() ? Unpacked{0, 0, sign} : Multiply(a, b);

    Unpacked c_unpacked = c.Unpack();
    c_unpacked.mantissa <<= kSignificandBits;

    return Add(product, c_unpacked, kGuard);
  }

  friend std::ostream& operator<<(std::ostream& output,
                                  const FloatingPoint& number) {
    if (number.IsNaN()) {
      output << "nan";
    } else if (number.IsInfinity()) {
      if (number.sign_) output << '-';
      output << "inf";
    } else {
      if (number.sign_) output << '-';

      if (number.exponent_ == 0) {
        if (number.mantissa_ == 0) {
          output << "0x0." << std::format("{:0{}x}", 0, kMantissaHex) << "p+0";
        } else {
          const uint32_t zeros =
              std::countl_zero(static_cast<uint32_t>(number.mantissa_)) -
              (32 - kMantissaBits);
          const uint32_t normalized =
              number.mantissa_ << (zeros + 1) & kMantissaMask;
          output << "0x1."
                 << std::format("{:0{}x}", normalized << kMantissaShiftHex,
                                kMantissaHex)
                 << "p-" << (kBias + zeros);
        }
      } else {
        output << "0x1."
               << std::format("{:0{}x}", number.mantissa_ << kMantissaShiftHex,
                              kMantissaHex)
               << 'p';

        if ((number.exponent_ - kBias) & (1 << kExponentBits)) {
          output << '-' << (~(number.exponent_ - kBias) + 1);
        } else {
          output << '+' << number.exponent_ - kBias;
        }
      }
    }

    Type sign = static_cast<Type>(number.sign_)
                << (kMantissaBits + kExponentBits);
    Type exponent = static_cast<Type>(number.exponent_) << kMantissaBits;
    Type mantissa = number.mantissa_;

    output << " 0x"
           << std::format("{:0{}X}", sign | exponent | mantissa, kTotalHex);

    return output;
  }

 private:
  Type mantissa_ : kMantissaBits;
  Type exponent_ : kExponentBits;
  Type sign_ : 1;

  static constexpr uint32_t kSignificandBits = kMantissaBits + 1;
  static constexpr uint32_t kMantissaMask = (1 << kMantissaBits) - 1;
  static constexpr uint32_t kExponentMask = (1 << kExponentBits) - 1;
  static constexpr uint32_t kBias = (1 << (kExponentBits - 1)) - 1;
  static constexpr uint32_t kUnpackedShift = 2 * kMantissaBits + 1;
  static constexpr uint32_t kGuard = 3;

  static constexpr uint32_t kImplicit = 1U << kMantissaBits;
  static constexpr uint32_t kQuiet = 1U << (kMantissaBits - 1);

  static constexpr uint32_t kMantissaHex = (kMantissaBits + 4) / 4;
  static constexpr uint32_t kTotalHex =
      (kMantissaBits + kExponentBits + 1 + 3) / 4;
  static constexpr uint32_t kMantissaShiftHex =
      kMantissaHex * 4 - kMantissaBits;

  inline static Rounding rounding_ = Rounding::kTowardZero;

  struct Unpacked {
    uint64_t mantissa;
    int32_t exponent;
    uint32_t sign;
  };

  Unpacked Unpack() const {
    if (exponent_ == 0) {
      if (mantissa_ == 0) return {0, 0, static_cast<uint32_t>(sign_)};

      const uint32_t zeros =
          std::countl_zero(static_cast<uint32_t>(mantissa_)) -
          (32 - kMantissaBits);
      return {static_cast<uint64_t>(mantissa_) << (zeros + 1),
              -static_cast<int32_t>(zeros), static_cast<uint32_t>(sign_)};
    }

    return {static_cast<uint64_t>(mantissa_ | kImplicit),
            static_cast<int32_t>(exponent_), static_cast<uint32_t>(sign_)};
  }

  // Special values
  static FloatingPoint PositiveZero() { return {0, 0, 0}; }

  static FloatingPoint NegativeZero() { return {0, 0, 1}; }

  static FloatingPoint PositiveInfinity() { return {0, kExponentMask, 0}; }

  static FloatingPoint NegativeInfinity() { return {0, kExponentMask, 1}; }

  static FloatingPoint NaN() { return {kQuiet, kExponentMask, 1}; }

  // Special value checkers
  [[nodiscard]] bool IsZero() const { return exponent_ == 0 && mantissa_ == 0; }

  [[nodiscard]] bool IsInfinity() const {
    return exponent_ == kExponentMask && mantissa_ == 0;
  }

  [[nodiscard]] bool IsNaN() const {
    return exponent_ == kExponentMask && mantissa_ != 0;
  }

  static FloatingPoint MakeQuiet(const FloatingPoint& value) {
    return {static_cast<Type>(value.mantissa_ | kQuiet), value.exponent_,
            value.sign_};
  }

  static FloatingPoint Round(uint32_t mantissa, const uint64_t remainder,
                             const uint64_t divisor, int32_t exponent,
                             uint32_t sign) {
    bool should_round_up = false;

    if (exponent <= 0) {
      const uint32_t full = mantissa | kImplicit;
      const int32_t shift = 1 - exponent;

      if (shift > static_cast<int32_t>(kSignificandBits)) {
        if (full || remainder) {
          if (rounding_ == Rounding::kTowardPositiveInfinity)
            should_round_up = (sign == 0);
          else if (rounding_ == Rounding::kTowardNegativeInfinity)
            should_round_up = (sign != 0);
        }
        if (should_round_up) return {1, 0, static_cast<Type>(sign)};
        return sign ? NegativeZero() : PositiveZero();
      }

      const uint32_t shifted = full & ((1U << shift) - 1);
      uint32_t denormalized = full >> shift;
      const uint64_t combined_remainder =
          static_cast<uint64_t>(shifted) * divisor + remainder;
      const uint64_t combined_divisor = divisor << shift;

      if (combined_remainder != 0) {
        if (rounding_ == Rounding::kTowardNearestEven)
          should_round_up = combined_remainder * 2 > combined_divisor ||
                            (combined_remainder * 2 == combined_divisor &&
                             (denormalized & 1));
        else if (rounding_ == Rounding::kTowardPositiveInfinity)
          should_round_up = (sign == 0);
        else if (rounding_ == Rounding::kTowardNegativeInfinity)
          should_round_up = (sign != 0);
      }
      if (should_round_up) {
        ++denormalized;
        if (denormalized & kImplicit) return {0, 1, static_cast<Type>(sign)};
      }

      return {static_cast<Type>(denormalized), 0, static_cast<Type>(sign)};
    }

    if (remainder != 0) {
      if (rounding_ == Rounding::kTowardNearestEven)
        should_round_up = (remainder * 2 > divisor) ||
                          (remainder * 2 == divisor && mantissa & 1);
      else if (rounding_ == Rounding::kTowardPositiveInfinity)
        should_round_up = sign == 0;
      else if (rounding_ == Rounding::kTowardNegativeInfinity)
        should_round_up = sign != 0;
    }
    if (should_round_up) {
      ++mantissa;
      if (mantissa & kImplicit) {
        mantissa = 0;
        ++exponent;
      }
    }

    if (exponent >= static_cast<int32_t>(kExponentMask)) {
      if ((rounding_ == Rounding::kTowardNearestEven) ||
          (rounding_ == Rounding::kTowardPositiveInfinity && sign == 0) ||
          (rounding_ == Rounding::kTowardNegativeInfinity && sign != 0)) {
        return sign ? NegativeInfinity() : PositiveInfinity();
      }
      return {static_cast<Type>(kMantissaMask),
              static_cast<Type>(kExponentMask - 1), static_cast<Type>(sign)};
    }

    return {static_cast<Type>(mantissa), static_cast<Type>(exponent),
            static_cast<Type>(sign)};
  }

  static Unpacked Multiply(const FloatingPoint& a, const FloatingPoint& b) {
    Unpacked a_unpacked = a.Unpack();
    Unpacked b_unpacked = b.Unpack();
    uint64_t product = a_unpacked.mantissa * b_unpacked.mantissa;
    int32_t exponent =
        a_unpacked.exponent + b_unpacked.exponent - static_cast<int32_t>(kBias);

    if (product & (1ULL << kUnpackedShift)) {
      ++exponent;
    } else {
      product <<= 1;
    }

    return {product, exponent, static_cast<uint32_t>(a.sign_ ^ b.sign_)};
  }

  static FloatingPoint Add(Unpacked a, Unpacked b, uint32_t guard = 0) {
    const uint64_t msb = 1ULL << (kUnpackedShift + guard);
    const uint32_t remainder_bits = kSignificandBits + guard;

    uint64_t a_mantissa = a.mantissa << guard;
    uint64_t b_mantissa = b.mantissa << guard;

    if (a_mantissa == 0 && b_mantissa == 0) {
      if (a.sign == b.sign) return a.sign ? NegativeZero() : PositiveZero();
      return rounding_ == Rounding::kTowardNegativeInfinity ? NegativeZero()
                                                            : PositiveZero();
    }

    if (a_mantissa == 0) {
      return Round(
          static_cast<uint32_t>(b.mantissa >> kSignificandBits) & kMantissaMask,
          b.mantissa & ((1ULL << kSignificandBits) - 1),
          1ULL << kSignificandBits, b.exponent, b.sign);
    }

    if (b_mantissa == 0) {
      return Round(
          static_cast<uint32_t>(a.mantissa >> kSignificandBits) & kMantissaMask,
          a.mantissa & ((1ULL << kSignificandBits) - 1),
          1ULL << kSignificandBits, a.exponent, a.sign);
    }

    int32_t difference = a.exponent - b.exponent;
    int32_t exponent;

    if (difference > 0) {
      if (difference < 64) {
        const uint64_t lost = b_mantissa & ((1ULL << difference) - 1);
        b_mantissa >>= difference;
        if (lost) b_mantissa |= 1;
      } else {
        b_mantissa = 1;
      }
      exponent = a.exponent;
    } else if (difference < 0) {
      difference = -difference;
      if (difference < 64) {
        const uint64_t lost = a_mantissa & ((1ULL << difference) - 1);
        a_mantissa >>= difference;
        if (lost) a_mantissa |= 1;
      } else {
        a_mantissa = 1;
      }
      exponent = b.exponent;
    } else {
      exponent = a.exponent;
    }

    uint64_t mantissa;
    uint32_t sign;

    if (a.sign == b.sign) {
      mantissa = a_mantissa + b_mantissa;
      sign = a.sign;
    } else {
      if (a_mantissa >= b_mantissa) {
        mantissa = a_mantissa - b_mantissa;
        sign = a.sign;
      } else {
        mantissa = b_mantissa - a_mantissa;
        sign = b.sign;
      }
    }

    if (mantissa == 0) {
      return rounding_ == Rounding::kTowardNegativeInfinity ? NegativeZero()
                                                            : PositiveZero();
    }

    while (mantissa >= msb << 1) {
      mantissa = (mantissa >> 1) | (mantissa & 1);
      ++exponent;
    }
    while (!(mantissa & msb)) {
      mantissa <<= 1;
      --exponent;
    }

    const uint64_t remainder = mantissa & ((1ULL << remainder_bits) - 1);
    const uint64_t divisor = 1ULL << remainder_bits;

    return Round(
        static_cast<uint32_t>(mantissa >> remainder_bits) & kMantissaMask,
        remainder, divisor, exponent, sign);
  }
};