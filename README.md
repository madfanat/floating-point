# floating-point

__floating-point__ is a C++23 implementation of a templated floating-point arithmetic library following IEEE 754-2019, with a command-line interface.

## Usage

```sh
./floating-point <format> <rounding> <value>
./floating-point <format> <rounding> <operation> <a> <b>
./floating-point <format> <rounding> <operation> <a> <b> <c>
```

Values are hexadecimal, e.g. `0x3F800000` for single-precision `1.0`.

### Prebuilt Formats

| Format   | Description      | Total bits | Exponent bits | Fraction bits | Arithmetic |
| -------- | ---------------- | ---------- | ------------- | ------------- | ---------- |
| `s`      | Single precision | 32         | 8             | 23            | Yes        |
| `h`      | Half precision   | 16         | 5             | 10            | Yes        |
| `e4m3fn` | E4M3FN           | 8          | 4             | 3             | No         |
| `e5m2`   | E5M2             | 8          | 5             | 2             | No         |
| `e2m1`   | E2M1             | 4          | 2             | 1             | No         |

Each format has one sign bit. The low-precision formats support decoding and output only.

### Rounding

| Value | Mode                            |
| ----- | ------------------------------- |
| `0`   | Toward zero.                    |
| `1`   | To nearest, ties to even.        |
| `2`   | Toward positive infinity.       |
| `3`   | Toward negative infinity.       |

The rounding argument is required for every operation.

### Operations

| Operation | Arguments | Description                                              |
| --------- | --------- | ------------------------------------------------------   |
| `+`       | `a b`     | Addition.                                                |
| `-`       | `a b`     | Subtraction.                                             |
| `'*'`     | `a b`     | Multiplication.                                          |
| `/`       | `a b`     | Division.                                                |
| `mad`     | `a b c`   | `a * b + c`, rounding after multiplication and addition. |
| `fma`     | `a b c`   | `a * b + c`, rounding once after the operation.          |

Examples:

```sh
./floating-point s 1 0x3F800000
# 0x1.000000p+0 0x3F800000

./floating-point h 1 + 0x3C00 0x4000
# 0x1.800p+1 0x4200

./floating-point h 1 '*' 0x4145 0x142EB
# 0x1.23cp+3 0x488F

./floating-point s 2 / 0x1 0x0
# inf 0x7F800000

./floating-point s 1 mad 0x3DCCCCCD 0x41200000 0xBF800000
# 0x0.000000p+0 0x00000000

./floating-point s 1 fma 0x3DCCCCCD 0x41200000 0xBF800000
# 0x1.000000p-26 0x32800000

./floating-point e2m1 1 0xF
# -0x1.8p+2 0xF
```

### Output
```text
<value> <hex>
```

- `value`: normalized hexadecimal notation with lowercase digits and an explicitly signed decimal exponent. Zero is `0x0.`. Special values: `nan`, `-inf` and `inf`.
- `hex`: the bits with a `0x` prefix, uppercase digits, and leading zeros to fill the selected format.
- The fractional part: six hexadecimal digits for single precision, three for half precision, and one for the low-precision formats.

### Specifications

- The NaN operand survives. If multiple operands are NaNs, the first is used.
- Invalid operations on non-NaN operands produce a negative quiet NaN with the remaining fraction bits set to zero.

## Building

Build the project:

```sh
c++ -std=c++23 -O2 main.cpp -o floating-point
```

## Credits
- [RonoveRaum](https://github.com/RonoveRaum) — for the idea

## License
The application and library source code is licensed under the MIT License.