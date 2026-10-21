# Integer Multiplication Benchmark

This project contains the code to compare integer multiplication algorithms against each other and the GMP.
All code can be found in `Code/`.

The following implementations have been included in our research:

- Schoolbook multiplication
- FFT multiplication
- Karatsuba
- Open AI's cutting edge research algorithm
- GMP multiplication

Also in this repo are shared contracts and types, and the benchmarking harness.

## Interface

Every implementation exposes the interface `custom_mul_n(result, a, b, n)` on little endian `mp_limb_t` arrays (radix is 2^`GMP_LIMB_BITS`- each limb represents a value in this base), plus setup/teardown functions so scratch memory (the high speed, temporary storage area that the process uses) is allocated and deallocated outside the timed region.

## Prerequisites

- C99 compiler (gcc/clang)
- `make`
- and GMP with headers (you can install this on ubuntu with `sudo apt install build-essential libgmp-dev`).

This repo has been implemented and tested on ubuntu 22.04 but you can also use WSL or MSYS2: `pacman -S mingw-w64-x86_64-gmp`

## Build

```sh
cd Code
make
```

## Run

The benchmarking function can be configured in many different ways, as shown below:

```sh
./Benchmark [--mode isolated|e2e|both] [--sizes 16,64,256,...] [--iters N] [--warmup W] [--steps S] [--seed X] [--algos schoolbook,fft,gmp]
```

- isolated mode times repeated `custom_mul_n` calls on fixed-size random operands.
- **e2e**: times a chain of `--steps` modular squarings `x <- x^2 + 1 mod 2^(limbBits*n) - 1`, with the multiplication under test.
- Each run verifies results against GMP before timing, warms up, touches all buffers, and prints CSV:
  `mode,algorithm,n_limbs,iterations,total_ms,per_op_us`.
- `--iters 0` (default) auto-selects iterations to target ~0.25 s for isolated runs; e2e defaults to 5 iterations.
- Unimplemented algorithms (Karatsuba, OpenAI) are skipped.

Example: `./Benchmark --mode isolated --sizes 64,1024,16384 > results.csv`

The FFT uses 16-bit digits for n ≤ 2048 limbs and 8-bit digits above that to keep double precision rounding exact.
