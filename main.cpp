#include <cstdint>
#include <iostream>
#include <string>

#include "floating_point/formats.h"

enum class Format { kSinglePrecision, kHalfPrecision, kE4M3FN, kE5M2, kE2M1 };

int main(const int argc, char* argv[]) {
  if (argc != 4 && argc != 6 && argc != 7) {
    std::cerr << "Error: wrong amount of arguments." << '\n';
    return 1;
  }

  const std::string format_string = argv[1];

  Format format;
  if (format_string == "s") {
    format = Format::kSinglePrecision;
  } else if (format_string == "h") {
    format = Format::kHalfPrecision;
  } else if (format_string == "e4m3fn") {
    format = Format::kE4M3FN;
  } else if (format_string == "e5m2") {
    format = Format::kE5M2;
  } else if (format_string == "e2m1") {
    format = Format::kE2M1;
  } else {
    std::cerr << "Error: wrong format option." << '\n';
    return 1;
  }

  size_t rounding_index;
  try {
    rounding_index = std::stoi(argv[2]);
  } catch (...) {
    std::cerr << "Error: wrong rounding option." << '\n';
    return 1;
  }

  Rounding rounding;
  switch (rounding_index) {
    case 0:
      rounding = Rounding::kTowardZero;
      break;
    case 1:
      rounding = Rounding::kTowardNearestEven;
      break;
    case 2:
      rounding = Rounding::kTowardPositiveInfinity;
      break;
    case 3:
      rounding = Rounding::kTowardNegativeInfinity;
      break;
    default:
      std::cerr << "Error: wrong rounding option." << '\n';
      return 1;
  }

  SinglePrecision::SetRounding(rounding);
  HalfPrecision::SetRounding(rounding);
  E2M1::SetRounding(rounding);
  E4M3FN::SetRounding(rounding);
  E5M2::SetRounding(rounding);

  switch (argc) {
    case 4: {
      uint64_t value;
      try {
        value = std::stoul(argv[3], nullptr, 16);
      } catch (...) {
        std::cerr << "Error: wrong number." << '\n';
        return 1;
      }

      switch (format) {
        case Format::kSinglePrecision:
          std::cout << SinglePrecision(static_cast<uint32_t>(value)) << '\n';
          break;
        case Format::kHalfPrecision:
          std::cout << HalfPrecision(static_cast<uint32_t>(value)) << '\n';
          break;
        case Format::kE4M3FN:
          std::cout << E4M3FN(static_cast<uint32_t>(value)) << '\n';
          break;
        case Format::kE5M2:
          std::cout << E5M2(static_cast<uint32_t>(value)) << '\n';
          break;
        case Format::kE2M1:
          std::cout << E2M1(static_cast<uint32_t>(value)) << '\n';
          break;
      }
      break;
    }
    case 6: {
      const std::string operation = argv[3];

      uint64_t a_value;
      uint64_t b_value;

      try {
        a_value = std::stoul(argv[4], nullptr, 16);
      } catch (...) {
        std::cerr << "Error: wrong number." << '\n';
        return 1;
      }

      try {
        b_value = std::stoul(argv[5], nullptr, 16);
      } catch (...) {
        std::cerr << "Error: wrong number." << '\n';
        return 1;
      }

      switch (format) {
        case Format::kSinglePrecision: {
          const SinglePrecision a(static_cast<uint32_t>(a_value));
          const SinglePrecision b(static_cast<uint32_t>(b_value));

          if (operation == "+") {
            std::cout << a + b << '\n';
          } else if (operation == "-") {
            std::cout << a - b << '\n';
          } else if (operation == "*") {
            std::cout << a * b << '\n';
          } else if (operation == "/") {
            std::cout << a / b << '\n';
          } else {
            std::cerr << "Error: wrong operation type." << '\n';
            return 1;
          }
          break;
        }

        case Format::kHalfPrecision: {
          const HalfPrecision a(static_cast<uint32_t>(a_value));
          const HalfPrecision b(static_cast<uint32_t>(b_value));

          if (operation == "+") {
            std::cout << a + b << '\n';
          } else if (operation == "-") {
            std::cout << a - b << '\n';
          } else if (operation == "*") {
            std::cout << a * b << '\n';
          } else if (operation == "/") {
            std::cout << a / b << '\n';
          } else {
            std::cerr << "Error: wrong operation type." << '\n';
            return 1;
          }
          break;
        }

        default: {
          std::cerr << "Error: unsupported operation type." << '\n';
          return 1;
        }
      }
      break;
    }
    case 7: {
      const std::string operation = argv[3];

      uint64_t a_value;
      uint64_t b_value;
      uint64_t c_value;

      try {
        a_value = std::stoul(argv[4], nullptr, 16);
      } catch (...) {
        std::cerr << "Error: wrong number." << '\n';
        return 1;
      }

      try {
        b_value = std::stoul(argv[5], nullptr, 16);
      } catch (...) {
        std::cerr << "Error: wrong number." << '\n';
        return 1;
      }

      try {
        c_value = std::stoul(argv[6], nullptr, 16);
      } catch (...) {
        std::cerr << "Error: wrong number." << '\n';
        return 1;
      }

      switch (format) {
        case Format::kSinglePrecision: {
          const SinglePrecision a(static_cast<uint32_t>(a_value));
          const SinglePrecision b(static_cast<uint32_t>(b_value));
          const SinglePrecision c(static_cast<uint32_t>(c_value));

          if (operation == "mad") {
            std::cout << SinglePrecision::Mad(a, b, c) << '\n';
          } else if (operation == "fma") {
            std::cout << SinglePrecision::Fma(a, b, c) << '\n';
          } else {
            std::cerr << "Error: wrong operation type." << '\n';
            return 1;
          }
          break;
        }

        case Format::kHalfPrecision: {
          const HalfPrecision a(static_cast<uint32_t>(a_value));
          const HalfPrecision b(static_cast<uint32_t>(b_value));
          const HalfPrecision c(static_cast<uint32_t>(c_value));

          if (operation == "mad") {
            std::cout << HalfPrecision::Mad(a, b, c) << '\n';
          } else if (operation == "fma") {
            std::cout << HalfPrecision::Fma(a, b, c) << '\n';
          } else {
            std::cerr << "Error: wrong operation type." << '\n';
            return 1;
          }
          break;
        }

        default: {
          std::cerr << "Error: unsupported operation type." << '\n';
          return 1;
        }
      }
      break;
    }
  }

  return 0;
}
